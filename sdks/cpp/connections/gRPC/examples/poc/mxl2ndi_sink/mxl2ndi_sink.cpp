#include "device.mxl2ndi_sink.yaml.h"

#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

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
std::atomic<int> liveIndexChanged = -1;
std::atomic<int> displayIndexChanged = -1;
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
        if (flowId.empty()) {
            LOG(WARNING) << "Skipping input with empty flow ID";
            continue;
        }
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

    std::unique_ptr<catena::common::IParam> inputsParam = dm.getParam("/inputs", statusErr);
    catena::common::ParamWithValue<mxl2ndi_sink::Inputs>* inputs =
      dynamic_cast<catena::common::ParamWithValue<mxl2ndi_sink::Inputs>*>(inputsParam.get());

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

    st2138::StructList* flowDefs = value.mutable_struct_array_values();
    flowDefs->clear_struct_values();
    for (const std::unique_ptr<mxlcpp::MxlReader>& reader : readers) {
        std::unique_ptr<st2138::Value> flowDefValue = reader->getFlowDef();
        st2138::StructValue flowDefStruct = flowDefValue->struct_value();
        flowDefs->mutable_struct_values()->Add(std::move(flowDefStruct));
    }
    dm.setValue("/flow_defs", value);

    int readerIndex = -1;
    int updateDisplayIndex = -1;
    indexChanged = true;
    isRunning = true;
    bool updateParams = false;
    while (isRunning) {
        {
            std::lock_guard<std::mutex> lock(dm.mutex());
            int liveIndex = liveIndexChanged.exchange(-1);
            if (liveIndex >= 0) {
                if (liveIndex < 0 || liveIndex >= readers.size()) {
                    LOG(WARNING) << "Invalid live index: " << liveIndex;
                } else {
                    readerIndex = liveIndex;
                    updateDisplayIndex = liveIndex;
                    updateParams = true;
                }
            }
            int displayIndex = displayIndexChanged.exchange(-1);
            if (displayIndex >= 0) {
                if (displayIndex < 0 || displayIndex >= readers.size()) {
                    LOG(WARNING) << "Invalid display index: " << displayIndex;
                } else {
                    updateDisplayIndex = displayIndex;
                    updateParams = true;
                }
            }
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
                updateDisplayIndex = loadIndex;
            }
            if (flowIdChanged.exchange(false)) {
                indexChanged = false;
                std::string loadFlowId = selectedFlowId->get();
                bool found = false;
                for (size_t i = 0; i < readers.size(); ++i) {
                    if (readers[i]->getFlowId() == loadFlowId) {
                        readerIndex = static_cast<int>(i);
                        updateDisplayIndex = readerIndex;
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
            {
                std::lock_guard<std::mutex> lock(dm.mutex());
                selectedIndex->get() = readerIndex;
                selectedFlowId->get() = currentReader.getFlowId();
                selectedName->get() = currentReader.getName();
                dm.getValueSetByServer().emit("/selected_index", selectedIndexParam.get());
                dm.getValueSetByServer().emit("/selected_flow_id", selectedFlowIdParam.get());
                dm.getValueSetByServer().emit("/selected_name", selectedNameParam.get());
                for (size_t i = 0; i < inputs->get().size(); ++i) {
                    inputs->get()[i].display = (i == static_cast<size_t>(updateDisplayIndex)) ? 0 : 1;
                    inputs->get()[i].live = (i == static_cast<size_t>(readerIndex)) ? 0 : 1;
                }
                dm.getValueSetByServer().emit("/inputs", inputsParam.get());
            }
            std::unique_ptr<st2138::Value> flowDefValue = readers[updateDisplayIndex]->getFlowDef();
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
    } else if (fqoid.starts_with("/inputs/")) {
        // get the index from the OID
        size_t nextSlash = fqoid.find('/', 8); // length of "/inputs/" is 8
        if (nextSlash != std::string::npos) {
            std::string indexStr = fqoid.substr(8, nextSlash - 8);
            try {
                int index = std::stoi(indexStr);
                // live or display depends on endswith
                if (fqoid.ends_with("/live")) {
                    liveIndexChanged = index;
                } else if (fqoid.ends_with("/display")) {
                    displayIndexChanged = index;
                }
            } catch (const std::exception& e) {
                LOG(ERROR) << "Failed to parse index from OID: " << fqoid << " Error: " << e.what();
            }
        }
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
        ServiceConfig config = ServiceConfig().set_cq(cq.get()).add_dm(&dm);
        ServiceImpl service(config);

        // Updating device's default max array length.
        dm.set_default_max_length(config::default_max_array_size);

        builder.RegisterService(&service);


        globalServer = builder.BuildAndStart();
        LOG(INFO) << "GRPC on " << addr << " secure mode: " << config::secure_comms;

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
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger::init("one_of_everything");


    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, "0.0.0.0:" + std::to_string(config::port));
    catenaRpcThread.join();

    return 0;
}
