#include "device.mxl2ndi_sink.yaml.h"

#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

#include <absl/flags/parse.h>
#include <absl/flags/flag.h>
#include <absl/flags/usage.h>
#include <absl/strings/str_format.h>
#include <absl/strings/escaping.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <google/protobuf/util/json_util.h>

#include <signal.h>

#include <mxl/flow.h>
#include <mxl/mxl.h>
#include <mxl/time.h>

#include <Processing.NDI.Advanced.h>

#include <cstdint>
#include <cstring>

// std
#include <atomic>
#include <filesystem>
#include <thread>

#include "mxl_reader.hpp"

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;

std::unique_ptr<Server> globalServer = nullptr;
std::atomic<bool> isRunning = false;
std::atomic<bool> indexChanged = false;
std::atomic<bool> flowIdChanged = false;
std::unique_ptr<std::thread> flowThread = nullptr;
// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        LOG(INFO) << "Caught signal " << sig << ", shutting down";
        isRunning = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void RunVideoFlow() {
    if (!NDIlib_initialize()) {
        LOG(ERROR) << "Failed to initialize NDI library";
        return;
    }

    NDIlib_send_create_t NDI_send_create_desc;
    NDI_send_create_desc.p_ndi_name = "Catena MXL Sink";
    NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
    if (pNDI_send == nullptr) {
        LOG(ERROR) << "Failed to create NDI sender";
        return;
    }

    st2138::Value value;
    std::vector<std::unique_ptr<mxlcpp::MxlReader>> readers;
    catena::exception_with_status statusErr = dm.getValue("/inputs", value);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get inputs: " << statusErr.what();
        return;
    }
    for (const auto& input : value.struct_array_values().struct_values()) {
        std::string name = input.fields().at("name").string_value();
        std::string domain = input.fields().at("domain").string_value();
        std::string flowId = input.fields().at("flow_id").string_value();
        readers.emplace_back(std::make_unique<mxlcpp::MxlReader>(name, domain, flowId));
    }

    if (readers.empty()) {
        LOG(ERROR) << "No inputs configured";
        return;
    }

    statusErr = dm.getValue("/selected_index", value);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get selected index: " << statusErr.what();
        return;
    }

    value.set_string_value("Running");
    dm.setValue("/status", value);

    std::unique_ptr<catena::common::IParam> selectedIndexParam = dm.getParam("/selected_index", statusErr);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get selected index param: " << statusErr.what();
        return;
    }
    catena::common::ParamWithValue<int32_t>* selectedIndex =
      dynamic_cast<catena::common::ParamWithValue<int32_t>*>(selectedIndexParam.get());
    std::unique_ptr<catena::common::IParam> selectedFlowIdParam = dm.getParam("/selected_flow_id", statusErr);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get selected flow ID param: " << statusErr.what();
        return;
    }
    catena::common::ParamWithValue<std::string>* selectedFlowId =
      dynamic_cast<catena::common::ParamWithValue<std::string>*>(selectedFlowIdParam.get());
    std::unique_ptr<catena::common::IParam> selectedNameParam = dm.getParam("/selected_name", statusErr);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get selected name param: " << statusErr.what();
        return;
    }
    catena::common::ParamWithValue<std::string>* selectedName =
      dynamic_cast<catena::common::ParamWithValue<std::string>*>(selectedNameParam.get());

    int readerIndex = -1;
    indexChanged = true;
    isRunning = true;
    bool updateParams = false;
    while (isRunning) {
        {
            std::lock_guard<std::mutex> lock(dm.mutex());
            if (indexChanged.exchange(false)) {
                flowIdChanged = false;
                updateParams = true;
                int32_t loadIndex = selectedIndex->get();
                if (loadIndex < 0 || loadIndex >= readers.size()) {
                    LOG(WARNING) << "Invalid input index: " << loadIndex;
                    mxlSleepForNs(100'000'000);  // 100ms
                    continue;
                }
                readerIndex = loadIndex;
            }
            if (flowIdChanged.exchange(false)) {
                indexChanged = false;
                std::string loadFlowId = selectedFlowId->get();
                bool found = false;
                for (size_t i = 0; i < readers.size(); ++i) {
                    if (readers[i]->getFlowId() == loadFlowId) {
                        readerIndex = static_cast<int>(i);
                        found = true;
                        updateParams = true;
                        break;
                    }
                }
                if (!found) {
                    LOG(WARNING) << "Selected flow ID not found: " << selectedFlowId;
                    mxlSleepForNs(100'000'000);  // 100ms
                    continue;
                }
            }
        }

        mxlcpp::MxlReader& currentReader = *readers[readerIndex];

        if (updateParams) {
            updateParams = false;
            selectedIndex->get() = readerIndex;
            selectedFlowId->get() = currentReader.getFlowId();
            selectedName->get() = currentReader.getName();
            dm.getValueSetByServer().emit("/selected_index", selectedIndexParam.get());
            dm.getValueSetByServer().emit("/selected_flow_id", selectedFlowIdParam.get());
            dm.getValueSetByServer().emit("/selected_name", selectedNameParam.get());
            std::unique_ptr<st2138::Value> flowDefValue = readers[readerIndex]->getFlowDef();
            statusErr = dm.setValue("/flow_def", *flowDefValue);
            if (statusErr.status != catena::StatusCode::OK) {
                LOG(ERROR) << "Failed to set flow definition: " << statusErr.what();
                isRunning = false;
                break;
            }
        }

        // do the frame
        uint64_t rateNs = 0;
        const NDIlib_video_frame_v2_t* ndiFrame = currentReader.dumpNdiFrame(rateNs);
        if (ndiFrame == nullptr) {
            // No frame available, sleep a bit
            mxlSleepForNs(10'000'000);  // 10ms
            continue;
        }
        NDIlib_send_send_video_v2(pNDI_send, ndiFrame);

        mxlSleepForNs(rateNs);
    }

    value.set_string_value("Stopped");
    dm.setValue("/status", value);

    NDIlib_send_destroy(pNDI_send);
    NDIlib_destroy();
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> listFlowsCommand = dm.getCommand("/list_flows", err);
    listFlowsCommand->defineCommand(
      [](const st2138::Value& value,
         const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
          return std::make_unique<ParamDescriptor::CommandResponder>(
            [](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                st2138::Value domainsValue;
                st2138::CommandResponse response;
                catena::exception_with_status err = dm.getValue("/inputs/domains", domainsValue);
                // If the state parameter does not exist, return an exception
                if (err.status != catena::StatusCode::OK) {
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details(err.what());
                    co_return response;
                }
                std::string domain;
                std::istringstream domainStream(domainsValue.string_value());
                std::ostringstream flowIdsStream;
                while (std::getline(domainStream, domain, ';')) {
                    std::filesystem::path path{domain};
                    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
                        LOG(WARNING) << "Domain path does not exist or is not a directory: " << domain;
                        continue;
                    }
                    for (const auto& entry : std::filesystem::directory_iterator(path)) {
                        if (!entry.is_directory())
                            continue;
                        if (entry.path().extension() != ".mxl-flow")
                            continue;
                        std::string flowId = entry.path().stem().string();
                        if (flowIdsStream.tellp() != std::ostringstream::pos_type(0)) {
                            flowIdsStream << ";";
                        }
                        flowIdsStream << flowId;
                        LOG(INFO) << "Found flow ID: " << flowId << " in domain: " << domain;
                    }
                }
                st2138::Value flowIdsValue;
                flowIdsValue.set_string_value(flowIdsStream.str());
                dm.setValue("/flow_ids", flowIdsValue);
                response.mutable_no_response();
                co_return response;
            }(value, respond));
      });

    std::unique_ptr<IParam> startCommand = dm.getCommand("/start", err);
    startCommand->defineCommand(
      [](const st2138::Value& value,
         const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
          return std::make_unique<ParamDescriptor::CommandResponder>(
            [](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                st2138::CommandResponse response;
                if (isRunning) {
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is already running");
                    co_return response;
                }

                isRunning = true;
                flowThread = std::make_unique<std::thread>(RunVideoFlow);

                response.mutable_no_response();
                co_return response;
            }(value, respond));
      });

    std::unique_ptr<IParam> stopCommand = dm.getCommand("/stop", err);
    stopCommand->defineCommand(
      [](const st2138::Value& value,
         const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
          return std::make_unique<ParamDescriptor::CommandResponder>(
            [](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                st2138::CommandResponse response;
                if (!isRunning) {
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is not running");
                    co_return response;
                }

                isRunning = false;
                if (flowThread && flowThread->joinable()) {
                    flowThread->join();
                }

                response.mutable_no_response();
                co_return response;
            }(value, respond));
      });
}

void valueChangedCallback(const std::string& fqoid, const catena::common::IParam* param) {
    LOG(INFO) << "Value changed callback for OID: " << fqoid;
    if (fqoid == "/selected_index") {
        indexChanged = true;
    } else if (fqoid == "/selected_flow_id") {
        flowIdChanged = true;
    }
}

void RunRPCServer(std::string addr) {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        grpc::ServerBuilder builder;
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig()
                                 .set_EOPath(absl::GetFlag(FLAGS_static_root))
                                 .set_authz(absl::GetFlag(FLAGS_authz))
                                 .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
                                 .set_cq(cq.get())
                                 .add_dm(&dm);
        ServiceImpl service(config);

        // Updating device's default max array length.
        dm.set_default_max_length(absl::GetFlag(FLAGS_default_max_array_size));

        builder.RegisterService(&service);


        globalServer = builder.BuildAndStart();
        LOG(INFO) << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // start the heartbeat on the device
        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();

        dm.getValueSetByClient().connect(valueChangedCallback);

        // wait for the server to shutdown and tidy up
        globalServer->Wait();
        dm.stopHeartbeat();

        if (flowThread && flowThread->joinable()) {
            flowThread->join();
        }

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception& why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    Logger::init("one_of_everything");

    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    return 0;
}
