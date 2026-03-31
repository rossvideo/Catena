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
std::unique_ptr<std::thread> recvThread = nullptr;
std::unique_ptr<std::thread> flowThread = nullptr;
std::atomic<bool> refreshInProgress = false;
std::atomic<bool> shutdownToken = false;
std::atomic<uint32_t> currentSource = 0;
std::mutex bufferMutex;
std::atomic<uint32_t> readIndex = 0;  // index the MXL writer reads from
uint8_t* videoBuffer[2];              // double buffering between reader and writer threads
std::atomic<int32_t> frameWidth = 1920;
std::atomic<int32_t> frameHeight = 1080;

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

    catena::exception_with_status status{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> statusParam = dm.getParam("/refresh_status", status);
    ParamWithValue<std::string>* statusValue = dynamic_cast<ParamWithValue<std::string>*>(statusParam.get());
    statusValue->get() = "Refreshing";
    dm.getValueSetByServer().emit("/refresh_status", statusParam.get());

    NDIlib_find_create_t find_create;
    st2138::Value value;
    dm.getValue("/ndi_source_ips", value);
    find_create.p_extra_ips = value.string_value().c_str();
    NDIlib_find_instance_t finder = NDIlib_find_create_v2(&find_create);
    if (!finder) {
        return "Unable to create NDI finder with provided IPs";
    }

    uint32_t num_sources = 0;
    const NDIlib_source_t* sources = nullptr;
    do {
        // loop until we don't find anything new
        do {
            LOG(INFO) << "Searching for NDI sources...";
        } while (!shutdownToken && NDIlib_find_wait_for_sources(finder, 1000));
        sources = NDIlib_find_get_current_sources(finder, &num_sources);
        LOG(INFO) << "Found " << num_sources << " NDI sources";
        // wait until we have *something*
    } while (!shutdownToken && (force && num_sources == 0));

    if (shutdownToken) {
        refreshInProgress = false;
        NDIlib_find_destroy(finder);
        return "Refresh cancelled";
    }

    // update the source parameter
    std::unique_ptr<IParam> sourcesParam = dm.getParam("/ndi_sources", status);
    ParamWithValue<ndi2mxl::Ndi_sources>* sourcesValue =
      dynamic_cast<ParamWithValue<ndi2mxl::Ndi_sources>*>(sourcesParam.get());
    sourcesValue->get().clear();

    // internal list of source structs
    ndiSources.clear();

    // go through the source and add them to our lists
    for (uint32_t i = 0; i < num_sources; i++) {
        ndiSources.push_back({sources[i].p_ndi_name, sources[i].p_url_address});
        sourcesValue->get().push_back(ndi2mxl::Ndi_sources_elem{sources[i].p_ndi_name, 1 /* live */});
    }

    // clean up finder
    sources = nullptr;
    NDIlib_find_destroy(finder);

    // update the current source
    dm.getValue("/selected_ndi_source", value);
    if (num_sources > 0 && !value.string_value().empty()) {
        std::lock_guard<std::mutex> lock(dm.mutex());
        for (int i = 0; i < ndiSources.size(); i++) {
            if (ndiSources[i].name == value.string_value()) {
                LOG(INFO) << "Restoring current source index to " << i;
                sourcesValue->get()[i].live = 0;  // mark as live
                currentSource.store(i);
                break;
            }
        }
    }

    // finish up
    dm.getValueSetByServer().emit("/ndi_sources", sourcesParam.get());
    statusValue->get() = "Found " + std::to_string(num_sources) + " source(s)";
    dm.getValueSetByServer().emit("/refresh_status", statusParam.get());
    refreshInProgress = false;

    return "";
}

void RunNdiRecv() {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    // things to cleanup on exit
    NDIlib_recv_instance_t ndi_recv = nullptr;

    auto cleanup = [&]() {
        if (ndi_recv) {
            NDIlib_recv_destroy(ndi_recv);
            ndi_recv = nullptr;
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

    std::unique_ptr<IParam> sourcesParam = dm.getParam("/ndi_sources", status);
    ParamWithValue<ndi2mxl::Ndi_sources>* sourcesValue =
      dynamic_cast<ParamWithValue<ndi2mxl::Ndi_sources>*>(sourcesParam.get());
    std::unique_ptr<IParam> selectedSourceParam = dm.getParam("/selected_ndi_source", status);
    ParamWithValue<std::string>* selectedSourceValue =
      dynamic_cast<ParamWithValue<std::string>*>(selectedSourceParam.get());

    uint32_t current = -1;

    NDIlib_source_t source;
    source.p_ndi_name = ndiSources[currentSource.load()].name.c_str();
    source.p_url_address = ndiSources[currentSource.load()].url.c_str();

    NDIlib_recv_create_v3_t recvCreateDesc;
    recvCreateDesc.color_format = NDIlib_recv_color_format_UYVY_BGRA;

    NDIlib_video_frame_v2_t videoFrame;

    // stuff for printing the frame rate
    const int32_t windowSize = 100;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    std::deque<std::chrono::nanoseconds> frameTimes;
    std::chrono::milliseconds logInterval(5000);
    std::chrono::steady_clock::time_point lastLogTime = lastTime;

    while (!shutdownToken && isRunning) {
        // check if we need to change sources
        uint32_t loadCurrent = currentSource.load();
        if (loadCurrent >= ndiSources.size()) {
            LOG(WARNING) << "Current source index " << loadCurrent << " is out of range, resetting to 0";
            currentSource.store(0);
        } else if (loadCurrent != current) {
            current = loadCurrent;
            source.p_ndi_name = ndiSources[current].name.c_str();
            source.p_url_address = ndiSources[current].url.c_str();
            LOG(INFO) << "Switching to NDI source: " << source.p_ndi_name;
            if (ndi_recv) {
                NDIlib_recv_destroy(ndi_recv);
                ndi_recv = nullptr;
            }
            ndi_recv = NDIlib_recv_create_v3(&recvCreateDesc);
            NDIlib_recv_connect(ndi_recv, &source);
            {
                std::lock_guard<std::mutex> lock(dm.mutex());
                for (int32_t i = 0; i < sourcesValue->get().size(); i++) {
                    if (i == current) {
                        sourcesValue->get()[i].live = 0;
                    } else {
                        sourcesValue->get()[i].live = 1;
                    }
                }
                selectedSourceValue->get() = source.p_ndi_name;
                dm.getValueSetByServer().emit("/selected_ndi_source", selectedSourceParam.get());
                dm.getValueSetByServer().emit("/ndi_sources", sourcesParam.get());
            }
        }

        NDIlib_frame_type_e recvResult =
          NDIlib_recv_capture_v2(ndi_recv, &videoFrame, nullptr, nullptr, 10000);
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (recvResult == NDIlib_frame_type_none) {
            LOG(WARNING) << "No frame received within timeout";
        } else if (recvResult == NDIlib_frame_type_error) {
            LOG(ERROR) << "Error receiving frame";
            break;
        } else if (recvResult == NDIlib_frame_type_video) {
            // write to the buffer the MXL writer is NOT reading
            uint32_t writeIndex = 1 - readIndex.load();
            convertFrame(videoFrame.p_data, videoBuffer[writeIndex],
                         std::min(static_cast<int32_t>(videoFrame.xres), frameWidth.load()),
                         std::min(static_cast<int32_t>(videoFrame.yres), frameHeight.load()),
                         videoFrame.line_stride_in_bytes);

            // swap: tell the MXL writer to read from the freshly written buffer
            {
                std::lock_guard<std::mutex> lock(bufferMutex);
                readIndex.store(writeIndex);
            }

            NDIlib_recv_free_video_v2(ndi_recv, &videoFrame);
            frameTimes.emplace_back(now - lastTime);
            lastTime = now;
            if (frameTimes.size() > windowSize) {
                frameTimes.pop_front();
            }
        }
        if (now - lastLogTime > logInterval) {
            if (frameTimes.empty()) {
                LOG(INFO) << "Current NDI source: " << source.p_ndi_name << " no frames received yet";
            } else {
                double avgFrameTime =
                  std::accumulate(frameTimes.begin(), frameTimes.end(), std::chrono::nanoseconds(0)).count() /
                  static_cast<double>(frameTimes.size());
                double fps = 1e9 / avgFrameTime;
                LOG(INFO) << "Current NDI source: " << source.p_ndi_name << " actual FPS: " << fps
                          << " (based on " << frameTimes.size() << " frames)";
            }
            lastLogTime = now;
        }
    }
    isRunning = false;
    cleanup();
}

void RunVideoFlow() {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> statusParam = dm.getParam("/status", status);
    ParamWithValue<std::string>* statusValue = dynamic_cast<ParamWithValue<std::string>*>(statusParam.get());

    // things to cleanup on exit
    mxlInstance instance = nullptr;
    mxlFlowWriter writer = nullptr;

    auto cleanup = [&]() {
        if (writer) {
            mxlReleaseFlowWriter(instance, writer);
            writer = nullptr;
        }
        if (instance) {
            mxlDestroyInstance(instance);
            instance = nullptr;
        }
    };

    std::unique_ptr<IParam> createFlowParam = dm.getParam("/create_flow", status);
    ParamWithValue<ndi2mxl::Create_flow>* createFlowValue =
      dynamic_cast<ParamWithValue<ndi2mxl::Create_flow>*>(createFlowParam.get());

    std::unique_ptr<IParam> flowDefParam = dm.getParam("/flow_def", status);
    ParamWithValue<ndi2mxl::Flow_def>* flowDefValue =
      dynamic_cast<ParamWithValue<ndi2mxl::Flow_def>*>(flowDefParam.get());

    frameWidth = createFlowValue->get().width;
    frameHeight = createFlowValue->get().height;
    createFlowDef(createFlowValue->get(), flowDefValue->get());
    std::string flowDefJson = createVideoFlowJson(flowDefValue->get());
    LOG(INFO) << "Generated flow definition: " << flowDefJson;
    dm.getValueSetByServer().emit("/flow_def", flowDefParam.get());

    mxlRational videoGrainRate = {createFlowValue->get().numerator, createFlowValue->get().denominator};

    instance = mxlCreateInstance(createFlowValue->get().domain.c_str(), nullptr);
    if (!instance) {
        LOG(ERROR) << "Failed to create MXL instance";
        cleanup();
        return;
    }
    bool created;
    mxlStatus mxl_status =
      mxlCreateFlowWriter(instance, flowDefJson.c_str(), nullptr, &writer, nullptr, &created);
    if (mxl_status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to create MXL flow writer: " << mxl_status;
        cleanup();
        return;
    }
    LOG(INFO) << "MXL flow writer created successfully, created=" << created;

    statusValue->get() = "Running";
    dm.getValueSetByServer().emit("/status", statusParam.get());
    int32_t goodFrames = 0;
    uint64_t currentIndex = mxlGetCurrentIndex(&videoGrainRate);

    // init the buffer, need to make the frames v210 width x height
    // which is (width / 6) * 16 for 6 pixels per group and 16 bytes per group,
    // times the height for the full frame size
    uint32_t bufferSize = ((flowDefValue->get().frame_width / 6) * 16) * flowDefValue->get().frame_height;
    LOG(INFO) << "Allocating video buffers with size " << bufferSize << " bytes each";
    for (int i = 0; i < 2; i++) {
        videoBuffer[i] = new uint8_t[bufferSize];
        std::memset(videoBuffer[i], 0, bufferSize);
    }
    readIndex.store(0);

    // start the recv thread
    recvThread = std::make_unique<std::thread>(RunNdiRecv);

    // stuff for printing the frame rate
    const int32_t windowSize = 100;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    std::deque<std::chrono::nanoseconds> frameTimes;
    std::chrono::milliseconds logInterval(5000);
    std::chrono::steady_clock::time_point lastLogTime = lastTime;

    // get the time
    currentIndex = mxlGetCurrentIndex(&videoGrainRate);
    while (isRunning && !shutdownToken) {
        mxlGrainInfo gInfo;
        uint8_t* mxl_buffer = nullptr;

        mxl_status = mxlFlowWriterOpenGrain(writer, currentIndex, &gInfo, &mxl_buffer);
        if (mxl_status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to open grain for writing at index " << currentIndex << ": " << mxl_status;
            break;
        }

        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            gInfo.validSlices = gInfo.totalSlices;
            memcpy(mxl_buffer, videoBuffer[readIndex.load()], bufferSize);
        }

        mxl_status = mxlFlowWriterCommitGrain(writer, &gInfo);
        if (mxl_status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to commit grain at index " << currentIndex << ": " << mxl_status;
            break;
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        frameTimes.emplace_back(now - lastTime);
        lastTime = now;
        if (frameTimes.size() > windowSize) {
            frameTimes.pop_front();
        }
        if (now - lastLogTime > logInterval) {
            double avgFrameTime =
              std::accumulate(frameTimes.begin(), frameTimes.end(), std::chrono::nanoseconds(0)).count() /
              static_cast<double>(frameTimes.size());
            double fps = 1e9 / avgFrameTime;
            LOG(INFO) << "MXL writer index " << currentIndex << " committed, actual FPS: " << fps;
            lastLogTime = now;
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
    if (fqoid.starts_with("/ndi_sources/")) {
        // ignore changes while refresh is in progress to avoid conflicts with the refresh logic
        if (refreshInProgress) {
            LOG(INFO) << "Refresh in progress, ignoring value change for OID: " << fqoid;
            return;
        }
        // get the index from the OID
        size_t nextSlash = fqoid.find('/', 13);  // length of "/ndi_sources/" is 13
        if (nextSlash != std::string::npos) {
            std::string indexStr = fqoid.substr(13, nextSlash - 13);
            try {
                int index = std::stoi(indexStr);
                LOG(INFO) << "Setting current source index to " << index;
                currentSource.store(index);
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

        dm.getValueSetByClient().connect(valueChangedCallback);

        // wait for the server to shutdown and tidy up
        globalServer->Wait();
        dm.stopHeartbeat();

        if (flowThread && flowThread->joinable()) {
            flowThread->join();
        }

        if (recvThread && recvThread->joinable()) {
            recvThread->join();
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
