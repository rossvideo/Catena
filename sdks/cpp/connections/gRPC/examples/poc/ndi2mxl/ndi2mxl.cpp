#include "device.ndi2mxl.yaml.h"

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

#include "helpers.hpp"

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;

std::unique_ptr<Server> globalServer = nullptr;
std::atomic<bool> isRunning = false;
std::unique_ptr<std::thread> flowThread = nullptr;
std::atomic<bool> refreshInProgress = false;
std::atomic<bool> shutdownToken = false;

struct NdiSource {
    std::string name;
    std::string url;
};
std::vector<NdiSource> ndiSources;
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

std::string handleRefresh(bool force = false) {
    bool inProgress = refreshInProgress.exchange(true);
    if (inProgress) {
        return "Refresh already in progress";
    }
    NDIlib_find_create_t find_create;
    st2138::Value extraIpsValue;
    dm.getValue("/ndi_source_ips", extraIpsValue);
    find_create.p_extra_ips = extraIpsValue.string_value().c_str();
    NDIlib_find_instance_t finder = NDIlib_find_create_v2(&find_create);
    if (!finder) {
        return "Unable to create NDI finder with provided IPs";
    }

    std::vector<NdiSource> discovered;
    bool found = false;
    do {
        found = NDIlib_find_wait_for_sources(finder, 5000);
        if (found) {
            uint32_t num_sources = 0;
            // NOTE: existing pointers from a previous get_current_sources call are
            // invalidated here, so we copy immediately and don't hold NDIlib_source_t.
            const NDIlib_source_t* sources = NDIlib_find_get_current_sources(finder, &num_sources);
            LOG(INFO) << "Number of NDI sources found: " << num_sources;
            discovered.clear();
            for (uint32_t j = 0; j < num_sources; j++) {
                LOG(INFO) << "Source " << j << ": " << sources[j].p_ndi_name << " at "
                          << sources[j].p_url_address;
                discovered.push_back({sources[j].p_ndi_name, sources[j].p_url_address});
            }
        } else {
            LOG(INFO) << "No change in NDI sources detected within timeout";
        }
        // if force is true, keep trying until we find sources or the process is shutting down
    } while (!found || (force && discovered.empty() && !shutdownToken));

    // Finder is destroyed here; safe because we copied all strings above.
    NDIlib_find_destroy(finder);

    if (shutdownToken) {
        // just get outta here if we're shutting down, no need to update params or anything
        refreshInProgress = false;
        return "Refresh cancelled";
    }

    ndiSources = std::move(discovered);

    catena::exception_with_status status{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> sourcesParam = dm.getParam("/ndi_sources", status);
    ParamWithValue<std::vector<std::string>>* sourcesValue =
      dynamic_cast<ParamWithValue<std::vector<std::string>>*>(sourcesParam.get());
    sourcesValue->get().clear();
    for (const NdiSource& source : ndiSources) {
        sourcesValue->get().push_back(source.name);
    }
    sourcesValue->get().push_back("DummySource");  // for testing purposes
    dm.getValueSetByServer().emit("/ndi_sources", sourcesParam.get());
    refreshInProgress = false;
    return "";
}

void RunVideoFlow() {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> statusParam = dm.getParam("/status", status);
    ParamWithValue<std::string>* statusValue = dynamic_cast<ParamWithValue<std::string>*>(statusParam.get());

    // things to cleanup on exit
    NDIlib_recv_instance_t ndi_recv = nullptr;
    mxlInstance instance = nullptr;
    mxlFlowWriter writer = nullptr;

    auto cleanup = [&]() {
        if (ndi_recv) {
            NDIlib_recv_destroy(ndi_recv);
            ndi_recv = nullptr;
        }
        if (writer) {
            mxlReleaseFlowWriter(instance, writer);
            writer = nullptr;
        }
        if (instance) {
            mxlDestroyInstance(instance);
            instance = nullptr;
        }
    };

    std::string error = handleRefresh(true);
    if (!error.empty()) {
        LOG(ERROR) << "Failed to discover NDI sources: " << error;
        cleanup();
        return;
    }

    if (ndiSources.empty()) {
        LOG(ERROR) << "No NDI sources found";
        cleanup();
        return;
    }

    NDIlib_source_t source;
    source.p_ndi_name = ndiSources[0].name.c_str();
    source.p_url_address = ndiSources[0].url.c_str();

    NDIlib_recv_create_v3_t recvCreateDesc;
    recvCreateDesc.color_format = NDIlib_recv_color_format_UYVY_BGRA;

    ndi_recv = NDIlib_recv_create_v3(&recvCreateDesc);

    NDIlib_recv_connect(ndi_recv, &source);

    // get a frame to determine the frame format and other metadata
    // to build the mxl flow def
    NDIlib_video_frame_v2_t videoFrame;
    int recvResult = NDIlib_frame_type_none;
    while (recvResult != NDIlib_frame_type_video && !shutdownToken) {
        recvResult = NDIlib_recv_capture_v2(ndi_recv, &videoFrame, nullptr, nullptr, 5000);
        if (recvResult == NDIlib_frame_type_none) {
            LOG(INFO) << "No frame received within timeout while waiting for first frame";
        } else if (recvResult == NDIlib_frame_type_error) {
            LOG(ERROR) << "Error receiving frame while waiting for first frame";
            cleanup();
            return;
        }
    }
    if (recvResult != NDIlib_frame_type_video) {
        LOG(ERROR) << "First frame received is not a video frame";
        cleanup();
        return;
    }

    std::unique_ptr<IParam> flowDomain = dm.getParam("/domain", status);
    ParamWithValue<std::string>* flowDomainValue =
      dynamic_cast<ParamWithValue<std::string>*>(flowDomain.get());
    std::unique_ptr<IParam> flowIdParam = dm.getParam("/flow_id", status);
    ParamWithValue<std::string>* flowIdValue = dynamic_cast<ParamWithValue<std::string>*>(flowIdParam.get());
    std::unique_ptr<IParam> flowLabelParam = dm.getParam("/flow_label", status);
    ParamWithValue<std::string>* flowLabelValue =
      dynamic_cast<ParamWithValue<std::string>*>(flowLabelParam.get());

    std::string flowDef = createVideoFlowJson(videoFrame, flowIdValue->get(), flowLabelValue->get());
    LOG(INFO) << "Generated flow definition: " << flowDef;

    mxlRational videoGrainRate = {videoFrame.frame_rate_N, videoFrame.frame_rate_D};

    // DON't FORGET TO FREE!! less catastrophic here, but still a memory leak if we don't
    NDIlib_recv_free_video_v2(ndi_recv, &videoFrame);

    LOG(INFO) << flowDomainValue->get();
    instance = mxlCreateInstance(flowDomainValue->get().c_str(), nullptr);
    if (!instance) {
        LOG(ERROR) << "Failed to create MXL instance";
        cleanup();
        return;
    }
    bool created;
    mxlStatus mxl_status =
      mxlCreateFlowWriter(instance, flowDef.c_str(), nullptr, &writer, nullptr, &created);
    if (mxl_status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to create MXL flow writer: " << mxl_status;
        cleanup();
        return;
    }
    LOG(INFO) << "MXL flow writer created successfully, created=" << created;

    statusValue->get() = "Running";
    dm.getValueSetByServer().emit("/status", statusParam.get());

    while (isRunning && !shutdownToken) {
        // no timeout, we are letting mxl drive the timing of the flow, so we want to capture frames as they come in and process them immediately
        // if we don't a frame mxl wants us to generate a nil grain still and move on to the next one
        int recvResult = NDIlib_recv_capture_v2(ndi_recv, &videoFrame, nullptr, nullptr, 0);
        // if (recvResult == NDIlib_frame_type_none) {
        //     LOG(INFO) << "No frame received within timeout";
        //     continue;
        // } else
        if (recvResult == NDIlib_frame_type_error) {
            LOG(ERROR) << "Error receiving frame";
            break;
        }

        uint64_t currentIndex = mxlGetCurrentIndex(&videoGrainRate);

        mxlGrainInfo gInfo;
        uint8_t* mxl_buffer = nullptr;

        mxl_status = mxlFlowWriterOpenGrain(writer, currentIndex, &gInfo, &mxl_buffer);
        if (mxl_status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to open grain for writing at index " << currentIndex << ": " << mxl_status;
            break;
        }

        // if we received a video frame, copy it into the mxl buffer
        if (recvResult == NDIlib_frame_type_video) {
            gInfo.validSlices = gInfo.totalSlices;
            convertFrame(videoFrame.p_data, mxl_buffer, videoFrame.xres, videoFrame.yres, videoFrame.line_stride_in_bytes);

            // DON't FORGET TO FREE!!
            NDIlib_recv_free_video_v2(ndi_recv, &videoFrame);
        } else {
            // if we didn't receive a video frame, write an invalid grain
            gInfo.validSlices = 0;
            gInfo.flags |= MXL_GRAIN_FLAG_INVALID;
            LOG(WARNING) << "Received non-video frame, writing invalid grain at index " << currentIndex;
        }

        mxl_status = mxlFlowWriterCommitGrain(writer, &gInfo);
        if (mxl_status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to commit grain at index " << currentIndex << ": " << mxl_status;
            break;
        }

        ++currentIndex;

        uint64_t nsUntilNextIndex = mxlGetNsUntilIndex(currentIndex, &videoGrainRate);
        mxlSleepForNs(nsUntilNextIndex);
    }

    statusValue->get() = "Stopped";
    dm.getValueSetByServer().emit("/status", statusParam.get());

    isRunning = false;
    cleanup();
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
                bool expected = false;
                if (!isRunning.compare_exchange_strong(expected, true)) {
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is already running");
                    co_return response;
                }

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

    std::unique_ptr<IParam> refreshCommand = dm.getCommand("/ndi_refresh", err);
    refreshCommand->defineCommand(
      [](const st2138::Value& value,
         const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
          return std::make_unique<ParamDescriptor::CommandResponder>(
            [](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                st2138::CommandResponse response;
                try {
                    std::string refreshResult = handleRefresh();
                    if (!refreshResult.empty()) {
                        response.mutable_exception()->set_type("Refresh Error");
                        response.mutable_exception()->set_details(refreshResult);
                    } else {
                        response.mutable_no_response();
                    }

                } catch (const std::exception& e) {
                    response.mutable_exception()->set_type("NDI Error");
                    response.mutable_exception()->set_details(e.what());
                }
                co_return response;
            }(value, respond));
      });
}

void valueChangedCallback(const std::string& fqoid, const catena::common::IParam* param) {
    LOG(INFO) << "Value changed callback for OID: " << fqoid;
}

void RunRPCServer(std::string addr) {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        grpc::ServerBuilder builder;
        // set some grpc options
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
    Logger::init("ndimxl");

    if (!NDIlib_initialize()) {
        LOG(ERROR) << "Failed to initialize NDI library";
        return -1;
    }


    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, "0.0.0.0:" + std::to_string(config::port));
    catenaRpcThread.join();

    NDIlib_destroy();

    return 0;
}
