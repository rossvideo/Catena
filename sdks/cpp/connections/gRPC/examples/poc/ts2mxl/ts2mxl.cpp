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
#include <fstream>

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
// ------------------------------------------------------------
// MANUAL SMPTE v210 PACKER
// ------------------------------------------------------------
size_t pack_v210(const AVFrame* f, uint8_t* dst, size_t maxSize) {
    const int W = f->width;
    const int H = f->height;

    const uint8_t* Y  = f->data[0];
    const uint8_t* Cb = f->data[1];
    const uint8_t* Cr = f->data[2];

    const int Ys = f->linesize[0];
    const int Cs = f->linesize[1];

    // v210 layout
    const int groups_per_row = (W + 5) / 6;
    const int rowBytes = ((groups_per_row * 16 + 127) / 128) * 128;
    const size_t needed = size_t(rowBytes) * H;

    if (maxSize < needed)
        return 0;

    uint8_t* row_start = dst;

    for (int y = 0; y < H; ++y) {
        const uint8_t* Yrow  = Y  + y * Ys;
        const uint8_t* Cbrow = Cb + (y / 2) * Cs;
        const uint8_t* Crrow = Cr + (y / 2) * Cs;

        uint32_t* out = reinterpret_cast<uint32_t*>(row_start);

        for (int g = 0; g < groups_per_row; ++g) {
            int x = g * 6;

            int x0 = (x + 0 < W) ? x + 0 : W - 1;
            int x1 = (x + 1 < W) ? x + 1 : W - 1;
            int x2 = (x + 2 < W) ? x + 2 : W - 1;
            int x3 = (x + 3 < W) ? x + 3 : W - 1;
            int x4 = (x + 4 < W) ? x + 4 : W - 1;
            int x5 = (x + 5 < W) ? x + 5 : W - 1;

            int cw = (W + 1) / 2;
            int c0 = (x / 2 + 0 < cw) ? x / 2 + 0 : cw - 1;
            int c1 = (x / 2 + 1 < cw) ? x / 2 + 1 : cw - 1;
            int c2 = (x / 2 + 2 < cw) ? x / 2 + 2 : cw - 1;

            // 8-bit → 10-bit
            uint16_t Cb0 = (uint16_t(Cbrow[c0]) << 2) & 0x3FF;
            uint16_t Y0  = (uint16_t(Yrow[x0])  << 2) & 0x3FF;
            uint16_t Cr0 = (uint16_t(Crrow[c0]) << 2) & 0x3FF;
            uint16_t Y1  = (uint16_t(Yrow[x1])  << 2) & 0x3FF;

            uint16_t Cb2 = (uint16_t(Cbrow[c1]) << 2) & 0x3FF;
            uint16_t Y2  = (uint16_t(Yrow[x2])  << 2) & 0x3FF;
            uint16_t Cr2 = (uint16_t(Crrow[c1]) << 2) & 0x3FF;
            uint16_t Y3  = (uint16_t(Yrow[x3])  << 2) & 0x3FF;

            uint16_t Cb4 = (uint16_t(Cbrow[c2]) << 2) & 0x3FF;
            uint16_t Y4  = (uint16_t(Yrow[x4])  << 2) & 0x3FF;
            uint16_t Cr4 = (uint16_t(Crrow[c2]) << 2) & 0x3FF;
            uint16_t Y5  = (uint16_t(Yrow[x5])  << 2) & 0x3FF;

            // v210 packing (little-endian)
            out[0] =  Cb0 | (Y0 << 10) | (Cr0 << 20);
            out[1] =  Y1  | (Cb2 << 10) | (Y2 << 20);
            out[2] =  Cr2 | (Y3 << 10) | (Cb4 << 20);
            out[3] =  Y4  | (Cr4 << 10) | (Y5 << 20);

            out += 4;
        }

        row_start += rowBytes;
    }

    return needed;
}
// ------------------------------------------------------------
// CREATE v210 FLOW JSON
// ------------------------------------------------------------
std::string createV210FlowJson(picojson::value const& info, std::string flow_id, std::string& out) {
    const auto& s = info.get("streams").get(0);

    int width = (int)s.get("width").get<double>();
    int height = (int)s.get("height").get<double>();

    std::string rate = s.get("r_frame_rate").get<std::string>();
    int fps_n = std::stoi(rate.substr(0, rate.find('/')));
    int fps_d = std::stoi(rate.substr(rate.find('/') + 1));

    picojson::object root;

    root["description"] = picojson::value("SMPTE v210 Source");
    root["id"] = picojson::value(flow_id);
    root["format"] = picojson::value("urn:x-nmos:format:video");
    root["label"] = picojson::value("v210 10-bit 4:2:2");
    root["media_type"] = picojson::value("video/v210");
    root["parents"] = picojson::value(picojson::array());

    // REQUIRED GROUP HINT TAG
    {
        picojson::object tags;
        picojson::array grp;
        grp.emplace_back(picojson::value("v210 Source:Video"));
        tags["urn:x-nmos:tag:grouphint/v1.0"] = picojson::value(grp);
        root["tags"] = picojson::value(tags);
    }

    picojson::object rateObj;
    rateObj["numerator"] = picojson::value((double)fps_n);
    rateObj["denominator"] = picojson::value((double)fps_d);
    root["grain_rate"] = picojson::value(rateObj);

    root["frame_width"] = picojson::value((double)width);
    root["frame_height"] = picojson::value((double)height);
    root["interlace_mode"] = picojson::value("progressive");
    root["colorspace"] = picojson::value("BT709");

    // Packed video → no components
    root["components"] = picojson::value(picojson::array());

    out = picojson::value(root).serialize(true);
    return flow_id;
}

// ------------------------------------------------------------
// CREATE DIRECTORY
// ------------------------------------------------------------
void makedir() {
    system("mkdir -p /dev/shm/mxl");
}

// ------------------------------------------------------------
// MAIN VIDEO THREAD
// ------------------------------------------------------------
void run() {
    isRunning = true;

    // Load info.json
    std::ifstream f("/dev/shm/info.json");
    if (!f.is_open()) {
        LOG(ERROR) << "Cannot open info.json";
        return;
    }

    std::string text, line;
    while (std::getline(f, line))
        text += line;

    picojson::value info;
    std::string err = picojson::parse(info, text);
    if (!err.empty()) {
        LOG(ERROR) << "Failed to parse info.json";
        return;
    }

    // MXL instance
    mxlInstance instance = mxlCreateInstance("/dev/shm/mxl", "");
    if (!instance) {
        LOG(ERROR) << "Cannot create MXL instance";
        return;
    }

    // Flow JSON
    mxlFlowConfigInfo cfg{};
    std::string flowJson;
    std::string flowId = "5fbec3b1-1b0f-417d-9059-8b94a47197ed";

    createV210FlowJson(info, flowId, flowJson);

    if (mxlCreateFlow(instance, flowJson.c_str(), nullptr, &cfg) != MXL_STATUS_OK) {
        LOG(ERROR) << "Flow creation failed";
        return;
    }

    mxlFlowWriter writer;
    if (mxlCreateFlowWriter(instance, flowId.c_str(), nullptr, &writer) != MXL_STATUS_OK) {
        LOG(ERROR) << "Writer creation failed";
        return;
    }

    // FFmpeg
    AVFormatContext* fmt = nullptr;
    avformat_open_input(&fmt, "/dev/shm/main.ts", nullptr, nullptr);
    avformat_find_stream_info(fmt, nullptr);

    int vid = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    AVStream* vst = fmt->streams[vid];

    const AVCodec* dec = avcodec_find_decoder(vst->codecpar->codec_id);
    AVCodecContext* decCtx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(decCtx, vst->codecpar);
    avcodec_open2(decCtx, dec, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVPacket pkt;
    av_init_packet(&pkt);

    // Frame rate
    std::string rf = info.get("streams").get(0).get("r_frame_rate").get<std::string>();
    int fps_n = std::stoi(rf.substr(0, rf.find('/')));
    int fps_d = std::stoi(rf.substr(rf.find('/') + 1));
    mxlRational rate{fps_n, fps_d};

    // Expected v210 size
    int rowBytes = ((decCtx->width + 5) / 6) * 16;
    int expectedSize = rowBytes * decCtx->height;

    LOG(INFO) << "Expected v210 size = " << expectedSize;

    // Main loop
    while (isRunning) {
        if (av_read_frame(fmt, &pkt) < 0) {
            av_seek_frame(fmt, vid, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(decCtx);
            continue;
        }

        if (pkt.stream_index != vid) {
            av_packet_unref(&pkt);
            continue;
        }

        if (avcodec_send_packet(decCtx, &pkt) == 0) {

            while (avcodec_receive_frame(decCtx, frame) == 0) {

                uint64_t idx = mxlGetCurrentIndex(&rate);

                mxlGrainInfo ginfo;
                uint8_t* payload = nullptr;

                mxlFlowWriterOpenGrain(writer, idx, &ginfo, &payload);

                size_t written = pack_v210(frame, payload, ginfo.grainSize);

                if (written != (size_t)expectedSize) {
                    LOG(ERROR) << "v210 size mismatch: wrote " << written;
                }

                ginfo.validSlices = ginfo.totalSlices;
                ginfo.flags = 0;

                mxlFlowWriterCommitGrain(writer, &ginfo);

                mxlSleepForNs(mxlGetNsUntilIndex(idx, &rate));
            }
        }

        av_packet_unref(&pkt);
    }

    av_frame_free(&frame);
    avcodec_free_context(&decCtx);
    avformat_close_input(&fmt);

    mxlReleaseFlowWriter(instance, writer);
    mxlDestroyFlow(instance, flowId.c_str());
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
