#include "device.mxl_info.yaml.h"

#include <ParamWithValue.h>
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/strings/str_format.h>
#include <absl/strings/escaping.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <google/protobuf/util/json_util.h>

#include <signal.h>

#include <mxl/flow.h>
#include <mxl/mxl.h>

// std
#include <atomic>
#include <thread>

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;

Server* globalServer = nullptr;
std::atomic<bool> isRunning = false;
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

    char buffer[1024];
    size_t bufferSize = sizeof(buffer);

    status = mxlGetFlowDef(instance, "5fbec3b1-1b0f-417d-9059-8b94a47197ed", buffer, &bufferSize);
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


        std::unique_ptr<Server> server(builder.BuildAndStart());
        LOG(INFO) << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        std::thread playThread(run);

        // start the heartbeat on the device
        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();

        // wait for the server to shutdown and tidy up
        server->Wait();
        dm.stopHeartbeat();

        playThread.join();

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

    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    return 0;
}
