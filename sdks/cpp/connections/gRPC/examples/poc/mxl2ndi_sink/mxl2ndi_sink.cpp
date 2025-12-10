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

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;


// Map 10-bit (0..1023) to 8-bit (0..255)
inline std::uint8_t ten_to_eight(std::uint16_t v10) {
    // Fast: drop 2 LSBs
    return static_cast<std::uint8_t>(v10 >> 2);
}

// Convert one line of MXL 10-bit 4:2:2 into UYVY 8-bit.
//
// Assumes MXL samples for 2 pixels are in the order: Y0, Cb0, Y1, Cr0
// and each sample is stored in a 16-bit word with only the lower 10 bits used.
void v210_to_uyvy_line(const std::uint8_t* src_v210, std::uint8_t* dst_uyvy,
                       int width_pixels  // e.g. 1920
) {
    const std::uint32_t* s = reinterpret_cast<const std::uint32_t*>(src_v210);
    std::uint8_t* d = dst_uyvy;

    auto put_pair = [&](std::uint16_t U10, std::uint16_t Y0_10, std::uint16_t V10, std::uint16_t Y1_10) {
        const std::uint8_t U8 = ten_to_eight(U10);
        const std::uint8_t V8 = ten_to_eight(V10);
        const std::uint8_t Y0 = ten_to_eight(Y0_10);
        const std::uint8_t Y1 = ten_to_eight(Y1_10);

        *d++ = U8;
        *d++ = Y0;
        *d++ = V8;
        *d++ = Y1;
    };

    int x = 0;
    while (x < width_pixels) {
        std::uint32_t w0 = *s++;
        std::uint32_t w1 = *s++;
        std::uint32_t w2 = *s++;
        std::uint32_t w3 = *s++;

        std::uint16_t Cb0 = (w0) & 0x3FF;
        std::uint16_t Y0 = (w0 >> 10) & 0x3FF;
        std::uint16_t Cr0 = (w0 >> 20) & 0x3FF;

        std::uint16_t Y1 = (w1) & 0x3FF;
        std::uint16_t Cb2 = (w1 >> 10) & 0x3FF;
        std::uint16_t Y2 = (w1 >> 20) & 0x3FF;

        std::uint16_t Cr2 = (w2) & 0x3FF;
        std::uint16_t Y3 = (w2 >> 10) & 0x3FF;
        std::uint16_t Cb4 = (w2 >> 20) & 0x3FF;

        std::uint16_t Y4 = (w3) & 0x3FF;
        std::uint16_t Cr4 = (w3 >> 10) & 0x3FF;
        std::uint16_t Y5 = (w3 >> 20) & 0x3FF;

        // Pixels 0–1
        put_pair(Cb0, Y0, Cr0, Y1);
        // Pixels 2–3
        put_pair(Cb2, Y2, Cr2, Y3);
        // Pixels 4–5
        put_pair(Cb4, Y4, Cr4, Y5);

        x += 6;
    }
}


std::unique_ptr<Server> globalServer = nullptr;
std::atomic<bool> isRunning = false;
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

std::string getDomain(catena::exception_with_status& status) {
    st2138::Value domainValue;
    status = dm.getValue("/inputs/current_domain", domainValue);
    if (status.status != catena::StatusCode::OK) {
        return std::string{};
    }
    return domainValue.string_value();
}

std::string getFlowId(catena::exception_with_status& status) {
    st2138::Value flowIdValue;
    status = dm.getValue("/inputs/current_flow_id", flowIdValue);
    if (status.status != catena::StatusCode::OK) {
        return std::string{};
    }
    return flowIdValue.string_value();
}

void setFlowDef(mxlInstance& instance, std::string& flowId) {
    st2138::Value value;
    char buffer[1024];
    size_t bufferSize = sizeof(buffer);
    mxlStatus status = mxlGetFlowDef(instance, flowId.c_str(), buffer, &bufferSize);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to get MXL flow definition: " << status;
    } else {
        LOG(INFO) << "Flow definition: " << buffer;
        picojson::value picoVal;
        std::string err = picojson::parse(picoVal, buffer);
        if (!err.empty()) {
            LOG(ERROR) << "Failed to parse flow definition JSON: " << err;
        } else {
            picojson::object& obj = picoVal.get<picojson::object>();
            //strings
            std::vector<std::string> strings{"id",         "description",    "format",    "label",
                                             "media_type", "interlace_mode", "colorspace"};
            for (const std::string& key : strings) {
                if (obj.contains(key)) {
                    value.set_string_value(obj[key].get<std::string>());
                    dm.setValue("/flow_def/" + key, value);
                }
            }
            // numbers
            std::vector<std::string> numbers{"frame_width", "frame_height"};
            for (const std::string& key : numbers) {
                if (obj.contains(key)) {
                    value.set_int32_value(static_cast<int32_t>(obj[key].get<double>()));
                    dm.setValue("/flow_def/" + key, value);
                }
            }
            // tags
            if (obj.contains("tags")) {
                picojson::object& tags = obj["tags"].get<picojson::object>();
                /* making this structure:
                "string_array_values": {
                    "struct_values": [{
                        "fields": {
                            "name": { "string_value": "NAME" },
                            "values": { "string_array_values": { "strings": ["VALUE1", "VALUE23"] } }
                        }
                    }]
                }
                */
                google::protobuf::RepeatedPtrField<st2138::StructValue>* structValues =
                  value.mutable_struct_array_values()->mutable_struct_values();
                for (const auto& [tagKey, tagValue] : tags) {
                    picojson::array tagArray = tagValue.get<picojson::array>();
                    st2138::Value nameValue;
                    nameValue.set_string_value(tagKey);
                    st2138::Value tagValues;
                    google::protobuf::RepeatedPtrField<std::string>* strings =
                      tagValues.mutable_string_array_values()->mutable_strings();
                    for (const auto& tagItem : tagArray) {
                        strings->Add()->assign(tagItem.get<std::string>());
                    }
                    google::protobuf::Map<std::string, st2138::Value>* tag =
                      structValues->Add()->mutable_fields();
                    tag->insert({"name", nameValue});
                    tag->insert({"values", tagValues});
                }
                dm.setValue("/flow_def/tags", value);
            }
            // grain rate
            if (obj.contains("grain_rate")) {
                picojson::object& grainRate = obj["grain_rate"].get<picojson::object>();
                if (grainRate.contains("numerator") && grainRate.contains("denominator")) {
                    int32_t numerator = static_cast<int32_t>(grainRate["numerator"].get<double>());
                    value.set_int32_value(numerator);
                    dm.setValue("/flow_def/grain_rate/numerator", value);
                    int32_t denominator = static_cast<int32_t>(grainRate["denominator"].get<double>());
                    value.set_int32_value(denominator);
                    dm.setValue("/flow_def/grain_rate/denominator", value);
                }
            }
            // components
            if (obj.contains("components")) {
                picojson::array components = obj["components"].get<picojson::array>();
                google::protobuf::RepeatedPtrField<st2138::StructValue>* structValues =
                  value.mutable_struct_array_values()->mutable_struct_values();
                for (const auto& compItem : components) {
                    picojson::object compObj = compItem.get<picojson::object>();
                    st2138::Value nameValue;
                    nameValue.set_string_value(compObj["name"].get<std::string>());
                    st2138::Value widthValue;
                    widthValue.set_int32_value(static_cast<int32_t>(compObj["width"].get<double>()));
                    st2138::Value heightValue;
                    heightValue.set_int32_value(static_cast<int32_t>(compObj["height"].get<double>()));
                    st2138::Value bitDepthValue;
                    bitDepthValue.set_int32_value(static_cast<int32_t>(compObj["bit_depth"].get<double>()));
                    google::protobuf::Map<std::string, st2138::Value>* compMap =
                      structValues->Add()->mutable_fields();
                    compMap->insert({"name", nameValue});
                    compMap->insert({"width", widthValue});
                    compMap->insert({"height", heightValue});
                    compMap->insert({"bit_depth", bitDepthValue});
                }
                dm.setValue("/flow_def/components", value);
            }
        }
    }
}

void run() {
    isRunning = true;
    st2138::Value value;
    mxlFlowInfo flowInfo;
    const std::string headIndex = "/flow_info/head_index";
    const std::string lastWrite = "/flow_info/last_write_time";
    const std::string lastRead = "/flow_info/last_read_time";

    mxlInstance instance = mxlCreateInstance("/dev/shm/mxl", "");
    if (instance == nullptr) {
        LOG(ERROR) << "Failed to create MXL instance";
        return;
    }

    mxlFlowReader flowReader;
    mxlStatus status = mxlCreateFlowReader(instance, "5fbec3b1-1b0f-417d-9059-8b94a47197ed", "", &flowReader);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to create MXL flow reader: " << status;
        mxlDestroyInstance(instance);
        return;
    }

    while (isRunning) {
        // update the ticker once per second, and emit the event
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // get the flow info
        status = mxlFlowReaderGetInfo(flowReader, &flowInfo);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to get MXL flow info: " << status;
            continue;
        }
        value.set_string_value(std::to_string(flowInfo.runtime.headIndex));
        dm.setValue(headIndex, value);
        // convert epoch nanoseconds to datetime string
        std::time_t lastWriteTime = flowInfo.runtime.lastWriteTime / 1000000000;
        std::string lastWriteStr = std::ctime(&lastWriteTime);
        // remove trailing newline from ctime
        lastWriteStr.pop_back();
        value.set_string_value(lastWriteStr);
        dm.setValue(lastWrite, value);
        std::time_t lastReadTime = flowInfo.runtime.lastReadTime / 1000000000;
        std::string lastReadStr = std::ctime(&lastReadTime);
        // remove trailing newline from ctime
        lastReadStr.pop_back();
        value.set_string_value(lastReadStr);
        dm.setValue(lastRead, value);
    }

    // clean up
    mxlReleaseFlowReader(instance, flowReader);
    mxlDestroyInstance(instance);
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

    NDIlib_video_frame_v2_t NDI_video_frame;
    NDI_video_frame.xres = 1920;
    NDI_video_frame.yres = 1080;

    NDI_video_frame.FourCC = NDIlib_FourCC_video_type_UYVY;  // 8-bit 4:2:2
    NDI_video_frame.frame_rate_N = 30000;
    NDI_video_frame.frame_rate_D = 1001;
    NDI_video_frame.picture_aspect_ratio = 16.0f / 9.0f;
    NDI_video_frame.frame_format_type = NDIlib_frame_format_type_progressive;

    // UYVY: 2 bytes per pixel (U Y0 V Y1 for each 2 pixels)
    NDI_video_frame.line_stride_in_bytes = NDI_video_frame.xres * 2;

    const size_t ndi_buffer_bytes =
      static_cast<size_t>(NDI_video_frame.yres) * static_cast<size_t>(NDI_video_frame.line_stride_in_bytes);

    NDI_video_frame.p_data = new std::uint8_t[ndi_buffer_bytes];

    catena::exception_with_status statusErr{"", catena::StatusCode::OK};
    std::string domain = getDomain(statusErr);
    if (statusErr.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Failed to get current domain: " << statusErr.what();
        return;
    }
    std::string flowId = getFlowId(statusErr);
    
    mxlInstance instance = mxlCreateInstance(domain.c_str(), "");
    if (instance == nullptr) {
        LOG(ERROR) << "Failed to create MXL instance";
        return;
    }

    setFlowDef(instance, flowId);

    mxlFlowReader flowReader;
    mxlStatus status = mxlCreateFlowReader(instance, flowId.c_str(), "", &flowReader);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to create MXL flow reader: " << status;
        mxlDestroyInstance(instance);
        return;
    }
    mxlFlowInfo flowInfo;
    status = mxlFlowReaderGetInfo(flowReader, &flowInfo);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to get MXL flow info: " << status;
        mxlReleaseFlowReader(instance, flowReader);
        mxlDestroyInstance(instance);
        return;
    }
    mxlRational rate = flowInfo.config.common.grainRate;
    uint32_t maxSyncBatchSizeHint = flowInfo.config.common.maxSyncBatchSizeHint;

    st2138::Value value;
    value.set_string_value("Running");
    dm.setValue("/status", value);

    uint64_t timeoutNs =
      static_cast<std::uint64_t>(1.0 * rate.denominator * (1'000'000'000.0 / rate.numerator) + 1'000'000ULL);
    uint64_t index = mxlGetCurrentIndex(&rate);
    while (isRunning) {
        mxlGrainInfo grainInfo;
        uint8_t* payload = nullptr;

        status = mxlFlowReaderGetGrain(flowReader, index, 1000000000, &grainInfo, &payload);
        if (status == MXL_ERR_OUT_OF_RANGE_TOO_EARLY) {
            // We are too early somehow, keep trying the same grain index
            mxlFlowReaderGetInfo(flowReader, &flowInfo);
            LOG(WARNING) << "Failed to get samples at index " << index << ": TOO EARLY. Last published "
                         << flowInfo.runtime.headIndex;
            continue;
        } else if (status == MXL_ERR_OUT_OF_RANGE_TOO_LATE) {
            mxlFlowReaderGetInfo(flowReader, &flowInfo);
            LOG(WARNING) << "Failed to get grain at index " << index << ": TOO LATE. Last published "
                         << flowInfo.runtime.headIndex;

            // Grain expired. Realign to current index. GStreamer repeats the last valid frame for missing data; consuming applications
            // should do the same.
            index = mxlGetCurrentIndex(&rate);
            continue;
        } else if (status == MXL_ERR_TIMEOUT) {
            // Timeout waiting for grain
            LOG(WARNING) << "Timeout waiting for grain at index " << index;
            continue;
        } else if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Unexpected error when reading the grain " << index << " with status "
                       << static_cast<int>(status) << ". Exiting.";
            break;
        }

        if (grainInfo.validSlices != grainInfo.totalSlices) {
            // Full frame not ready yet
            continue;
        }

        if (!(grainInfo.flags & MXL_GRAIN_FLAG_INVALID)) {
            // valid grain, process payload here
            // copy payload to NDI frame
            LOG(INFO) << grainInfo.grainSize << " bytes read for grain index " << grainInfo.index;


            // v210 is packed 10-bit 4:2:2
            const int width = NDI_video_frame.xres;   // 1920
            const int height = NDI_video_frame.yres;  // 1080

            // Stride of the v210 data in bytes (already includes padding)
            const size_t v210_stride_bytes = grainInfo.grainSize / height;

            const std::uint8_t* src = payload;

            for (int y = 0; y < height; ++y) {
                const std::uint8_t* src_line = src + y * v210_stride_bytes;
                std::uint8_t* dst_line = NDI_video_frame.p_data + y * NDI_video_frame.line_stride_in_bytes;

                v210_to_uyvy_line(src_line, dst_line, width);
            }


            // send NDI frame
            NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
        }

        index++;
        mxlSleepForNs(mxlGetNsUntilIndex(index, &rate));
    }

    value.set_string_value("Stopped");
    dm.setValue("/status", value);

    // clean up
    mxlReleaseFlowReader(instance, flowReader);
    mxlDestroyInstance(instance);

    free(NDI_video_frame.p_data);
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
