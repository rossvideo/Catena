#include "device.mp42mxl.yaml.h"

#include <ParamWithValue.h>
#include <ParamDescriptor.h>
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
#include <fstream>
#include <random>
// Filesystem for checking existing flow IDs in a domain
#include <filesystem>

// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// Some FFmpeg builds may not define AV_PIX_FMT_V210; fallback for compilation.
// Use a widely available 10-bit 4:2:2 pixel format for conversion

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;

Server* globalServer = nullptr;
std::atomic<bool> isRunning = false;
std::unique_ptr<std::thread> playThread = nullptr;

// handle SIGINT / SIGTERM
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
std::string& path2String(std::string path){
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> coolParam = dm.getParam(path, err);
    if (coolParam == nullptr) {
        throw err;
    }
    auto& param =*dynamic_cast<ParamWithValue<std::string>*>(coolParam.get());
    return param.get();
}
/* +-----------------------------------------------
   | FFparse function
   +----------------------------------------------- */
struct VideoInfo {
    int width;
    int height;
    AVRational framerate;
};

// VideoInfo ffparse(const std::string& filepath) {
//     VideoInfo info{0,0,{0,1}};
//     AVFormatContext* fmt_ctx = nullptr;
//     if (avformat_open_input(&fmt_ctx, filepath.c_str(), nullptr, nullptr) < 0) {
//         throw std::runtime_error("Could not open input file: " + filepath);
//     }
//     if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
//         avformat_close_input(&fmt_ctx);
//         throw std::runtime_error("Could not find stream info: " + filepath);
//     }
//     AVCodec* codec = nullptr;
//     int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
//     if (video_stream_index < 0) {
//         avformat_close_input(&fmt_ctx);
//         throw std::runtime_error("Could not find video stream in file: " + filepath);
//     }
//     AVStream* video_stream = fmt_ctx->streams[video_stream_index];
//     info.width = video_stream->codecpar->width;
//     info.height = video_stream->codecpar->height;
//     info.framerate = av_guess_frame_rate(fmt_ctx, video_stream, nullptr);
//     avformat_close_input(&fmt_ctx);
//     return info;
// }

// Generate a random UUID v4 string for flow IDs.
static std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xffffffff);

    uint32_t d0 = dist(gen);
    uint16_t d1 = static_cast<uint16_t>(dist(gen));
    uint16_t d2 = static_cast<uint16_t>(dist(gen)) & 0x0fff;
    d2 |= 0x4000; // version 4

    uint16_t d3 = static_cast<uint16_t>(dist(gen)) & 0x3fff;
    d3 |= 0x8000; // RFC 4122 variant

    uint16_t d4a = static_cast<uint16_t>(dist(gen));
    uint32_t d4b = dist(gen);

    return absl::StrFormat("%08x-%04x-%04x-%04x-%04x%08x",
                           d0, d1, d2, d3, d4a, d4b);
}

// Check whether a flow directory already exists within the given domain path
static bool flow_id_exists(const std::string& domainPath, const std::string& id) {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(domainPath, ec) || !fs::is_directory(domainPath, ec)) {
        LOG(WARNING) << "Domain path not found or not a directory: " << domainPath;
        return false;
    }

    for (const auto& entry : fs::directory_iterator(domainPath, ec)) {
        if (ec) {
            LOG(WARNING) << "Error iterating domain '" << domainPath << "': " << ec.message();
            break;
        }
        std::error_code iec;
        if (entry.is_directory(iec)) {
            if (entry.path().filename() == id) {
                return true;
            }
        }
    }
    return false;
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> startCommand = dm.getCommand("/start", err);
    startCommand->defineCommand(
      [](const st2138::Value& value,
         const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
          return std::make_unique<ParamDescriptor::CommandResponder>(
            [](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
                LOG(INFO) << "Start command received";
                /*
                    1 check if already running
                    2 check if the video file exists
                    3 make a flow with the flow id and domain from inputs
                        3.1 get domain from inputs
                        3.2 get flow id from inputs
                        3.3 use ffparse to get info from video file
                        3.4 create mxl flow in domain with flow id and video info
                    4 convert the mp4 to v210 and write to the flow in a thread on loop
                        4.1 read packets from ffmpeg
                        4.2 decode packets to frames
                        4.3 convert frames to v210
                        4.4 write v210 frames to mxl flow
                        4.5 wait for next frame time
                */
                st2138::CommandResponse response;
                if (isRunning) {
                    LOG(WARNING) << "Flow is already running";
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is already running");
                    co_return response;
                }
                // // check if video file exists
                auto& videoFilePath = path2String("/inputs/video_file_path");
                if(!std::ifstream(videoFilePath).good()){
                    LOG(ERROR) << "Video file does not exist: " << videoFilePath;
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Video file does not exist: " + videoFilePath);
                    // co_return response; //TODO: uncomment when the rest is done
                }
                // check if auto configure is set
                auto& autoConfig = path2String("/inputs/auto_configure");
                auto& domain = path2String("/inputs/domain");
                auto& flowId = path2String("/inputs/flow/id");
                LOG(INFO) << "Auto configure value: " << autoConfig;
                if(autoConfig == "1"){
                    LOG(INFO) << "Auto configure is set. Using default domain";
                    // Ensure the flow ID is set and unique within the domain by listing directories
                    auto ensure_unique_flow_id = [&](std::string& id) {
                        if (id.empty()) {
                            id = generate_uuid();
                        }
                        int attempts = 0;
                        while (flow_id_exists(domain, id)) {
                            LOG(WARNING) << "Flow id already exists in domain '" << domain
                                         << "': " << id << ". Generating a new one.";
                            id = generate_uuid();
                            if (++attempts > 1000) {
                                LOG(ERROR) << "Exceeded attempts to generate a unique flow id.";
                                break;
                            }
                        }
                        LOG(INFO) << "Using flow id: " << id;
                    };

                    ensure_unique_flow_id(flowId);
                    //use ffparse to get video info from the video file
                    //auto videoInfo = ffparse(videoFilePath);

                }
                // create flow
                
                
                LOG(INFO) << "Creating flow " << flowId << " in domain " << domain;
                // auto videoInfo = ffparse(videoFilePath.get());


                LOG(INFO) << "Starting flow playback thread";
                isRunning = true;
                // playThread = std::make_unique<std::thread>(run);

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
                LOG(INFO) << "Stop command received";
                /*
                    1 check if running
                    2 stop the thread
                    3 clean up flow in domain
                */
                st2138::CommandResponse response;
                if (!isRunning) {
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is not running");
                    co_return response;
                }

                isRunning = false;
                if (playThread && playThread->joinable()) {
                    playThread->join();
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

        // start the heartbeat on the device
        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();

        // wait for the server to shutdown and tidy up
        server->Wait();
        dm.stopHeartbeat();

        if (playThread && playThread->joinable()) {
            playThread->join();
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
    Logger::init("mp42mxl");

    defineCommands();

    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    return 0;
}
