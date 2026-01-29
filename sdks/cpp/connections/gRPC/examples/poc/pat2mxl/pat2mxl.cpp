#include "device.pat2mxl.yaml.h"

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

#include <cstdint>
#include <cstring>

// std
#include <atomic>
#include <thread>
#include <fstream>

#include <boost/process.hpp>

using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;
using grpc::Server;

using namespace catena::common;
namespace bp = boost::process;

std::string mxlPath;
Server* globalServer = nullptr;
std::atomic<bool> isRunning = false;
std::unique_ptr<std::thread> monThread = nullptr;

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

template <typename T> T& getParamAs(const std::string& oid) {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = dm.getParam(oid, err);
    if (err.status != catena::StatusCode::OK) {
        LOG(ERROR) << "Error getting param " << oid << ": " << err.what();
        throw err;
    }
    ParamWithValue<T>* paramWithValue = dynamic_cast<ParamWithValue<T>*>(param.get());
    if (!paramWithValue) {
        LOG(ERROR) << "Error casting param " << oid << " to ParamWithValue<" << typeid(T).name() << ">";
        throw catena::exception_with_status{"", catena::StatusCode::UNKNOWN};
    }
    return paramWithValue->get();
}

void run() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> inputsParam = dm.getParam("/inputs", err);
    ParamWithValue<pat2mxl::Inputs>* inputsValue = dynamic_cast<ParamWithValue<pat2mxl::Inputs>*>(inputsParam.get());

    std::string domain = inputsValue->get().target_domain;
    std::string flowId = inputsValue->get().target_flow.id;
    if (domain.empty()) {
        LOG(ERROR) << "No target domain specified in /inputs/target_domain";
        return;
    }
    if (flowId.empty()) {
        LOG(ERROR) << "No target flow ID specified in /inputs/target_flow/id";
        return;
    }
    


    int in_width, int in_height, mxlRational in_rate,
        bool in_progressive, std::string const& in_colorspace


    std::unique_ptr<IParam> statusParam = dm.getParam("/status", err);
    ParamWithValue<std::string>* statusValue = dynamic_cast<ParamWithValue<std::string>*>(statusParam.get());
    
    // std::string command =
    //   std::format("{} --input {} --domain {} --video-id {}", mxlPath, tsFile, domain, flowId);
    LOG(INFO) << "Starting MXL process: " << command;
    bp::child mxlProcess(command, bp::std_out > stdout, bp::std_err > stderr);
    isRunning = true;
    statusValue->get() = "Running";
    dm.getValueSetByServer().emit("/status", statusParam.get());
    while (isRunning && mxlProcess.running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    mxlProcess.terminate();
    mxlProcess.wait();
    isRunning = false;
    statusValue->get() = "Stopped";
    dm.getValueSetByServer().emit("/status", statusParam.get());
    LOG(INFO) << "MXL process exited";
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
                st2138::CommandResponse response;
                if (isRunning) {
                    LOG(WARNING) << "Flow is already running";
                    response.mutable_exception()->set_type("Invalid Command");
                    response.mutable_exception()->set_details("Flow is already running");
                    co_return response;
                }

                LOG(INFO) << "Starting flow playback thread";
                monThread = std::make_unique<std::thread>(run);
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
                if (monThread && monThread->joinable()) {
                    monThread->join();
                }
                monThread = nullptr;

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

        if (monThread && monThread->joinable()) {
            monThread->join();
        }

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception& why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

bool checkMxlPath() {
    char* mxlEnv = std::getenv("MXL_PATH");
    if (mxlEnv == nullptr) {
        LOG(ERROR) << "MXL_PATH environment variable not set, exiting";
        return true;
    }
    mxlPath = std::string(mxlEnv);
    std::filesystem::path mxlPathFs{mxlPath};
    if (!std::filesystem::exists(mxlPathFs)) {
        LOG(ERROR) << "MXL_PATH " << mxlPath << " does not exist, exiting";
        return true;
    }
    mxlPath = std::filesystem::absolute(mxlPathFs).string();
    std::string cmd = mxlPath + " --help";
    int result = bp::system(cmd, bp::std_out > bp::null, bp::std_err > stderr);
    if (result != 0) {
        LOG(ERROR) << "MXL_PATH " << mxlPath << " is not executable, exiting";
        return true;
    }

    LOG(INFO) << "Using MXL_PATH: " << mxlPath;
    return false;
}

int main(int argc, char* argv[]) {
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    Logger::init("ts2mxl");

    defineCommands();
    if (checkMxlPath()) {
        return 1;
    }

    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    return 0;
}
