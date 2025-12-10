#include "device.ts2mxl.yaml.h"

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
std::string createVideoFlowJson(std::string const& in_uri, int in_width, int in_height, mxlRational in_rate, bool in_progressive,
    std::string const& in_colorspace, std::string& out_flowDef)
    {
        auto root = picojson::object{};
        auto label = std::string{"Binary TS flow for "} + in_uri;
        root["description"] = picojson::value(label);

        root["id"] = picojson::value("5fbec3b1-1b0f-417d-9059-8b94a47197ed");
        root["tags"] = picojson::value(picojson::object());
        // Use video format to satisfy current MXL constraints.
        root["format"] = picojson::value("urn:x-nmos:format:video");
        root["label"] = picojson::value(label);
        root["parents"] = picojson::value(picojson::array());
        root["media_type"] = picojson::value("video/v210");

        auto tags = picojson::object{};
        auto groupHint = picojson::array{};
        groupHint.emplace_back(picojson::value{"Looping Source:Video"});
        tags["urn:x-nmos:tag:grouphint/v1.0"] = picojson::value(groupHint);
        root["tags"] = picojson::value(tags);

        auto grain_rate = picojson::object{};
        grain_rate["numerator"] = picojson::value(static_cast<double>(in_rate.numerator));
        grain_rate["denominator"] = picojson::value(static_cast<double>(in_rate.denominator));
        root["grain_rate"] = picojson::value(grain_rate);
        root["frame_width"] = picojson::value(static_cast<double>(in_width));
        root["frame_height"] = picojson::value(static_cast<double>(in_height));
        root["interlace_mode"] = picojson::value(in_progressive ? "progressive" : "interlaced_tff");
        root["colorspace"] = picojson::value(in_colorspace);

        auto components = picojson::array{};
        auto add_component = [&](std::string const& name, int w, int h)
        {
            auto comp = picojson::object{};
            comp["name"] = picojson::value(name);
            comp["width"] = picojson::value(static_cast<double>(w));
            comp["height"] = picojson::value(static_cast<double>(h));
            comp["bit_depth"] = picojson::value(10.0);
            components.emplace_back(comp);
        };

        add_component("Y", in_width, in_height);
        add_component("Cb", in_width / 2, in_height);
        add_component("Cr", in_width / 2, in_height);

        root["components"] = picojson::value(components);

        out_flowDef = picojson::value(root).serialize(true);
        return "5fbec3b1-1b0f-417d-9059-8b94a47197ed";
    }
void makedir(){
    system("rm -rf /dev/shm/mxl");
    system("mkdir -p /dev/shm/mxl");
}
void run() {
    isRunning = true;
    // grab the ts from /dev/shm/loop/writer.ts
    mxlInstance instance = mxlCreateInstance("/dev/shm/mxl", "");
    if (instance == nullptr) {
        LOG(ERROR) << "Failed to create MXL instance";
        return;
    }
    mxlFlowConfigInfo configInfo;
    std::string flowDef;
    createVideoFlowJson(
        "mxl://5fbec3b1-1b0f-417d-9059-8b94a47197ed",
        1920,
        1080,
        mxlRational{30000, 1001},
        true,
        "BT.709",
        flowDef);
    auto res = mxlCreateFlow(instance, flowDef.c_str(), nullptr, &configInfo);
    if (res != MXL_STATUS_OK)
    {
        LOG(ERROR) << "Failed to create MXL flow : " << res;
        return;
    }
    mxlFlowWriter flowWriter;
    res = mxlCreateFlowWriter(instance, "5fbec3b1-1b0f-417d-9059-8b94a47197ed", nullptr, &flowWriter);
    if (res != MXL_STATUS_OK)
    {
        LOG(ERROR) << "Failed to create MXL flow writer: " << res;
        return;
    }

    // Read MPEG-TS and write to MXL ring buffer as fixed-size grains.
    // Align read size to the flow writer's grain size for clean commits.
    uint64_t grain_index = 0;
    std::ifstream ts_file("/dev/shm/loop/writer.ts", std::ios::in | std::ios::binary);
    if (!ts_file.is_open()) {
        LOG(ERROR) << "Failed to open TS file: /dev/shm/loop/writer.ts";
        mxlReleaseFlowWriter(instance, flowWriter);
        mxlDestroyInstance(instance);
        isRunning = false;
        return;
    }else{
        LOG(INFO) << "Opened TS file: /dev/shm/loop/writer.ts";
    }

    // Open first grain to determine grain size; allocate buffer accordingly.
    mxlGrainInfo grainInfo;
    uint8_t* payload = nullptr;
    auto status = mxlFlowWriterOpenGrain(flowWriter, grain_index, &grainInfo, &payload);
    if (status != MXL_STATUS_OK) {
        LOG(ERROR) << "Failed to open initial grain for sizing: " << status;
        mxlReleaseFlowWriter(instance, flowWriter);
        mxlDestroyInstance(instance);
        isRunning = false;
        return;
    }
    const size_t grain_size = static_cast<size_t>(grainInfo.grainSize);
    std::unique_ptr<uint8_t[]> ts_buffer(new uint8_t[grain_size]);

    // Fill and commit the first grain using the sizing info
    {
        size_t totalRead = 0;
        while (totalRead < grain_size && isRunning) {
            ts_file.read(reinterpret_cast<char*>(ts_buffer.get() + totalRead), grain_size - totalRead);
            std::streamsize bytesRead = ts_file.gcount();
            if (bytesRead <= 0) {
                ts_file.clear();
                ts_file.seekg(0, std::ios::beg);
                continue;
            }
            totalRead += static_cast<size_t>(bytesRead);
        }
        if (!isRunning) {
            ts_file.close();
            mxlReleaseFlowWriter(instance, flowWriter);
            mxlDestroyInstance(instance);
            return;
        }
        memcpy(payload, ts_buffer.get(), grain_size);
        grainInfo.validSlices = grainInfo.totalSlices;
        grainInfo.flags = 0;
        status = mxlFlowWriterCommitGrain(flowWriter, &grainInfo);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to commit initial grain: " << status;
            ts_file.close();
            mxlReleaseFlowWriter(instance, flowWriter);
            mxlDestroyInstance(instance);
            isRunning = false;
            return;
        }
        grain_index++;
    }

    // Throttle loop logging to avoid excessive output
    std::uint64_t lastLoopLogNs = 0;
    const std::uint64_t loopLogIntervalNs = 5'000'000'000ULL; // 5 seconds

    while (isRunning)
    {
        // Ensure we fill exactly one grain worth of data; loop the file if needed.
        size_t totalRead = 0;
        while (totalRead < grain_size && isRunning) {
            ts_file.read(reinterpret_cast<char*>(ts_buffer.get() + totalRead), grain_size - totalRead);
            std::streamsize bytesRead = ts_file.gcount();
            if (bytesRead <= 0) {
                // Loop the file from the beginning and log the event (rate-limited).
                auto const nowNs = ::mxlGetTime();
                if (nowNs - lastLoopLogNs >= loopLogIntervalNs) {
                    LOG(INFO) << "Looping TS file: rewinding to start.";
                    lastLoopLogNs = nowNs;
                }
                ts_file.clear();
                ts_file.seekg(0, std::ios::beg);
                continue;
            }
            totalRead += static_cast<size_t>(bytesRead);
        }

        if (!isRunning) break;

        mxlGrainInfo grainInfoIter;
        uint8_t* payloadIter = nullptr;
        status = mxlFlowWriterOpenGrain(flowWriter, grain_index, &grainInfoIter, &payloadIter);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to open grain for writing: " << status;
            break;
        }

        // Copy exactly one grain worth of MPEG-TS data
        memcpy(payloadIter, ts_buffer.get(), grain_size);
        grainInfoIter.validSlices = grainInfoIter.totalSlices;
        grainInfoIter.flags = 0;

        status = mxlFlowWriterCommitGrain(flowWriter, &grainInfoIter);
        if (status != MXL_STATUS_OK) {
            LOG(ERROR) << "Failed to commit grain: " << status;
            break;
        }
        grain_index++;
    }
    ts_file.close();
    mxlReleaseFlowWriter(instance, flowWriter);
    mxlDestroyInstance(instance);
}


void RunRPCServer(std::string addr) {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    // SIGKILL cannot be caught; do not register a handler.

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
    Logger::init("ts2mxl");

    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
    makedir();
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    return 0;
}
