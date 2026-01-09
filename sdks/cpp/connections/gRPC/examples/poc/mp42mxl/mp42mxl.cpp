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

/* +-----------------------------------------------
   | FFparse function
   +----------------------------------------------- */
void ffparse(const std::string& videoFilePath) {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> flow = dm.getParam("/flow", err);
    if (flow == nullptr) {
        throw err;
    }
    auto& flowParam =*dynamic_cast<ParamWithValue<mp42mxl::Flow>*>(flow.get());
    mp42mxl::Flow& flowValue = flowParam.get();
    // flow id
    auto ensure_unique_flow_id = [&](std::string& id) {
        if (id.empty()) {
            id = generate_uuid();
        }
        int attempts = 0;
        while (flow_id_exists(path2String("/inputs/domain"), id)) {
            LOG(WARNING) << "Flow id already exists in domain '" << path2String("/inputs/domain")
                            << "': " << id << ". Generating a new one.";
            id = generate_uuid();
            if (++attempts > 1000) {
                LOG(ERROR) << "Exceeded attempts to generate a unique flow id.";
                break;
            }
        }
        LOG(INFO) << "Using flow id: " << id;
    };
    ensure_unique_flow_id(flowValue.id);
    if (flowValue.description.empty()) {
        flowValue.description = "Catena Managed MP4 to MXL flow ";
    }
    if (flowValue.tags.empty()) {
        mp42mxl::Flow::Tags_elem tag;
        tag.name = "source";
        tag.values.push_back("mp42mxl_poc");
        flowValue.tags.push_back(tag);
        tag.name = "urn:x-nmos:tag:grouphint\\/v1.0";
        tag.values.clear();
        tag.values.push_back("mp4 File Source:Video");
        flowValue.tags.push_back(tag);
    }

    if (flowValue.label.empty()) {
        flowValue.label = videoFilePath.substr(videoFilePath.find_last_of("/\\") + 1)+" flow";
    }
    if (flowValue.parents.empty()) {
        flowValue.parents.push_back("");
    }
    if (flowValue.media_type.empty()) {
        flowValue.media_type = "mp4 video file";
    }
    // Colorspace will be populated from FFmpeg probe below
    // Use FFmpeg to parse the video file and extract flow information
    AVFormatContext* fmt_ctx = nullptr;
    int ret = 0;
    auto ff_errstr = [](int err) -> std::string {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(err, buf, sizeof(buf));
        return std::string(buf);
    };

    if ((ret = avformat_open_input(&fmt_ctx, videoFilePath.c_str(), nullptr, nullptr)) < 0) {
        LOG(ERROR) << "avformat_open_input failed for '" << videoFilePath << "': " << ff_errstr(ret);
        return;
    }
    if ((ret = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        LOG(ERROR) << "avformat_find_stream_info failed: " << ff_errstr(ret);
        avformat_close_input(&fmt_ctx);
        return;
    }

    int vstream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (vstream_index < 0) {
        LOG(ERROR) << "No video stream found in '" << videoFilePath << "'";
        avformat_close_input(&fmt_ctx);
        return;
    }
    AVStream* vstream = fmt_ctx->streams[vstream_index];

    // Grain rate
    if (flowValue.grain_rate.numerator == 0 || flowValue.grain_rate.denominator == 0) {
        AVRational fr = vstream->avg_frame_rate.num && vstream->avg_frame_rate.den
                            ? vstream->avg_frame_rate
                            : vstream->r_frame_rate;
        if (fr.num == 0 || fr.den == 0) { fr.num = 0; fr.den = 1; }
        flowValue.grain_rate.numerator = fr.num;
        flowValue.grain_rate.denominator = fr.den;
    }

    // Decoder setup to get actual frame fields
    const AVCodec* dec = avcodec_find_decoder(vstream->codecpar->codec_id);
    if (!dec) {
        LOG(ERROR) << "Decoder not found for codec id: " << vstream->codecpar->codec_id;
        // Fallback to codecpar for size
        if (flowValue.frame_width == 0) flowValue.frame_width = vstream->codecpar->width;
        if (flowValue.frame_height == 0) flowValue.frame_height = vstream->codecpar->height;
        avformat_close_input(&fmt_ctx);
        return;
    }
    AVCodecContext* dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx) {
        LOG(ERROR) << "Failed to allocate codec context";
        avformat_close_input(&fmt_ctx);
        return;
    }
    if ((ret = avcodec_parameters_to_context(dec_ctx, vstream->codecpar)) < 0) {
        LOG(ERROR) << "avcodec_parameters_to_context failed: " << ff_errstr(ret);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return;
    }
    if ((ret = avcodec_open2(dec_ctx, dec, nullptr)) < 0) {
        LOG(ERROR) << "avcodec_open2 failed: " << ff_errstr(ret);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool got_frame = false;
    while (!got_frame && (ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
        if (pkt->stream_index == vstream_index) {
            if ((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) {
                LOG(WARNING) << "avcodec_send_packet failed: " << ff_errstr(ret);
                av_packet_unref(pkt);
                continue;
            }
            while ((ret = avcodec_receive_frame(dec_ctx, frame)) >= 0) {
                got_frame = true;
                break;
            }
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF && ret < 0) {
                LOG(WARNING) << "avcodec_receive_frame error: " << ff_errstr(ret);
            }
        }
        av_packet_unref(pkt);
    }

    if (got_frame) {
        if (flowValue.frame_width == 0) flowValue.frame_width = frame->width;
        if (flowValue.frame_height == 0) flowValue.frame_height = frame->height;

        // Interlace detection
        bool interlaced = (frame->flags & AV_FRAME_FLAG_INTERLACED) != 0;
        bool tff = (frame->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST) != 0;
        if (flowValue.interlace_mode.empty()) {
            flowValue.interlace_mode = interlaced ? (tff ? "interlaced_tff" : "interlaced_bff") : "progressive";
        }
        // Pixel format string
        {
            AVPixelFormat pf = static_cast<AVPixelFormat>(frame->format);
            const char* pf_name = av_get_pix_fmt_name(pf);
            if (pf_name && pf_name[0] != '\0') {
                flowValue.format = pf_name;
            } else if (flowValue.format.empty()) {
                flowValue.format = "unknown";
            }
        }
        // Colorspace
        auto map_colorspace = [](AVColorSpace cs) -> std::string {
            switch (cs) {
                case AVCOL_SPC_BT709: return "BT709";
                case AVCOL_SPC_BT2020_NCL:
                case AVCOL_SPC_BT2020_CL: return "BT2020";
                case AVCOL_SPC_BT470BG: return "BT601";
                case AVCOL_SPC_SMPTE170M: return "BT601";
                case AVCOL_SPC_SMPTE240M: return "SMPTE240M";
                case AVCOL_SPC_RGB: return "RGB";
                case AVCOL_SPC_UNSPECIFIED: return "unspecified";
                default: return "unknown";
            }
        };
        flowValue.colorspace = map_colorspace(frame->colorspace);

        // Components from pixel format
        if (flowValue.components.empty()) {
            AVPixelFormat pf = static_cast<AVPixelFormat>(frame->format);
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(pf);
            if (desc) {
                int bit_depth = desc->comp[0].depth;
                bool is_rgb = (desc->flags & AV_PIX_FMT_FLAG_RGB);
                bool has_alpha = (desc->flags & AV_PIX_FMT_FLAG_ALPHA);

                if (is_rgb) {
                    mp42mxl::Flow::Components_elem r{"R", frame->width, frame->height, bit_depth};
                    mp42mxl::Flow::Components_elem g{"G", frame->width, frame->height, bit_depth};
                    mp42mxl::Flow::Components_elem b{"B", frame->width, frame->height, bit_depth};
                    flowValue.components = {r, g, b};
                    if (has_alpha) {
                        flowValue.components.push_back(mp42mxl::Flow::Components_elem{"A", frame->width, frame->height, bit_depth});
                    }
                } else {
                    int cw = desc->log2_chroma_w;
                    int ch = desc->log2_chroma_h;
                    int wY = frame->width;
                    int hY = frame->height;
                    int wC = frame->width >> cw;
                    int hC = frame->height >> ch;
                    flowValue.components.push_back(mp42mxl::Flow::Components_elem{"Y", wY, hY, bit_depth});
                    flowValue.components.push_back(mp42mxl::Flow::Components_elem{"Cb", wC, hC, bit_depth});
                    flowValue.components.push_back(mp42mxl::Flow::Components_elem{"Cr", wC, hC, bit_depth});
                    if (has_alpha) {
                        flowValue.components.push_back(mp42mxl::Flow::Components_elem{"A", wY, hY, bit_depth});
                    }
                }
            }
        }
    } else {
        // Fallback to codec parameters if no frame decoded
        if (flowValue.frame_width == 0) flowValue.frame_width = vstream->codecpar->width;
        if (flowValue.frame_height == 0) flowValue.frame_height = vstream->codecpar->height;
        if (flowValue.format.empty()) {
            AVPixelFormat pf = static_cast<AVPixelFormat>(vstream->codecpar->format);
            const char* pf_name = av_get_pix_fmt_name(pf);
            flowValue.format = (pf_name && pf_name[0] != '\0') ? std::string(pf_name) : std::string("unknown");
        }
        // Colorspace fallback from codec context if available
        auto map_colorspace_fallback = [](AVColorSpace cs) -> std::string {
            switch (cs) {
                case AVCOL_SPC_BT709: return "BT709";
                case AVCOL_SPC_BT2020_NCL:
                case AVCOL_SPC_BT2020_CL: return "BT2020";
                case AVCOL_SPC_BT470BG: return "BT601";
                case AVCOL_SPC_SMPTE170M: return "BT601";
                case AVCOL_SPC_SMPTE240M: return "SMPTE240M";
                case AVCOL_SPC_RGB: return "RGB";
                case AVCOL_SPC_UNSPECIFIED: return "unspecified";
                default: return "unknown";
            }
        };
        flowValue.colorspace = map_colorspace_fallback(dec_ctx->colorspace);
    }

    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);

    dm.getValueSetByServer().emit("/flow", &flowParam);
}

mxlInstance instance = nullptr;


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
                        3.4 create mxl converted flow in domain
                    4 convert the input mp4 to v210 and write to the flow in a thread on loop
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
                    co_return response;
                }
                // check if auto configure is set
                auto& autoConfig = path2String("/inputs/auto_configure");
                auto& domain = path2String("/inputs/domain");
                LOG(INFO) << "Auto configure value: " << autoConfig;
                if(autoConfig == "1"){
                    LOG(INFO) << "Auto configure is set. Using default domain";
                    ffparse(videoFilePath);
                }
                // create flow
                // MXL instance
                std::filesystem::create_directories(domain);
                instance = mxlCreateInstance(domain.c_str(), "");
                if (!instance) {
                    LOG(ERROR) << "Cannot create MXL instance";
                    response.mutable_exception()->set_type("MXL ERROR");
                    response.mutable_exception()->set_details("could not create MXL instance for domain: " + domain);
                    co_return response;
                }
                // make a v210 mxl flow in the domain with the flow id
                mxlFlowConfigInfo cfg{};
                std::string flowJson;
                catena::exception_with_status err{"", catena::StatusCode::OK};
                std::unique_ptr<IParam> flow = dm.getParam("/flow", err);
                if (flow == nullptr) {
                    throw err;
                }
                auto& flowParam =*dynamic_cast<ParamWithValue<mp42mxl::Flow>*>(flow.get());
                mp42mxl::Flow& flowValue = flowParam.get();
                flowJson = absl::StrFormat(R"({
                    "id": "%s",
                    "description": "MP4 to MXL v210 flow",
                    "tags": {
                        "source": ["mp42mxl_poc"],
                        "urn:x-nmos:tag:grouphint/v1.0": ["mp4 File Source:Video"]
                    },
                    "format": "urn:x-nmos:format:video",
                    "label": "MP4 to MXL v210 flow",
                    "parents": [""],
                    "media_type": "video/v210",
                    "grain_rate": {
                        "numerator": %d,
                        "denominator": %d
                    },
                    "frame_width": %d,
                    "frame_height": %d,
                    "interlace_mode": "%s",
                    "colorspace": "BT709",
                    "components": [
                        {
                            "name": "Y",
                            "width": %d,
                            "height": %d,
                            "bit_depth": 10
                        },
                        {
                            "name": "Cb",
                            "width": %d,
                            "height": %d,
                            "bit_depth": 10
                        },
                        {
                            "name": "Cr",
                            "width": %d,
                            "height": %d,
                            "bit_depth": 10
                        }
                    ]
                })",
                    flowValue.id,
                    flowValue.grain_rate.numerator,
                    flowValue.grain_rate.denominator,
                    flowValue.frame_width,
                    flowValue.frame_height,
                    flowValue.interlace_mode,
                    flowValue.frame_width, flowValue.frame_height,
                    flowValue.frame_width / 2, flowValue.frame_height / 2,
                    flowValue.frame_width / 2, flowValue.frame_height / 2
                );

                

                if (mxlCreateFlow(
                        instance,
                        flowJson.c_str(),
                        nullptr,
                        &cfg
                    ) != MXL_STATUS_OK) {
                    LOG(ERROR) << "Flow creation failed" << flowJson;
                    response.mutable_exception()->set_type("MXL ERROR");
                    response.mutable_exception()->set_details("could not create MXL Flow for domain: " + flowValue.id);
                    co_return response;
                }
                
                LOG(INFO) << "Creating flow " << flowValue.id << " in domain " << domain;
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


                catena::exception_with_status err{"", catena::StatusCode::OK};
                std::unique_ptr<IParam> flow = dm.getParam("/flow", err);
                if (flow == nullptr) {
                    throw err;
                }
                auto& flowParam =*dynamic_cast<ParamWithValue<mp42mxl::Flow>*>(flow.get());
                mp42mxl::Flow& flowValue = flowParam.get();
                //remove mxl flow
                mxlDestroyFlow(instance, flowValue.id.c_str());
                mxlDestroyInstance(instance);
                instance = nullptr;

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
    //TODO: clean up mxl instance and flow if running on ctl+c
}
