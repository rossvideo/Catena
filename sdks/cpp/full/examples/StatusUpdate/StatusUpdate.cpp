
#include <ServiceImpl.h>
#include <utils.h>
#include <JSON.h>
#include <DeviceModel.h>
#include <ParamAccessor.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <full/service.grpc.pb.h>


#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"


#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>

using grpc::Server;


using DeviceModel = catena::full::DeviceModel;
using ParamAccessor = catena::full::ParamAccessor;

// set up the command line parameters
ABSL_FLAG(uint16_t, port, 6254, "Catena service port");
ABSL_FLAG(std::string, certs, "${HOME}/test_certs", "path/to/certs/files");
ABSL_FLAG(std::string, secure_comms, "off", "Specify type of secure comms, options are: \
  \"off\", \"ssl\", \"tls\"");
ABSL_FLAG(bool, mutual_authc, false, "use this to require client to authenticate");
ABSL_FLAG(bool, authz, false, "use OAuth token authorization");
ABSL_FLAG(std::string, device_model, "../../../example_device_models/device.minimal.json",
          "Specify the JSON device model to use.");
ABSL_FLAG(std::string, static_root, getenv("HOME"), "Specify the directory to search for external objects");

Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}


std::string timeNow() {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto now_micros = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_micros.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    auto now_c = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&now_c), "%F %T") << '.' << std::setw(6) << std::setfill('0')
       << micros.count() % 1000000;
    return ss.str();
}
// expand env variables
void expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}

// creates a Security Credentials object based on the command line options
std::shared_ptr<grpc::ServerCredentials> getServerCredentials() {
    std::shared_ptr<grpc::ServerCredentials> ans;
    if (absl::GetFlag(FLAGS_secure_comms).compare("off") == 0) {
        // run without secure comms
        ans = grpc::InsecureServerCredentials();
    } else if (absl::GetFlag(FLAGS_secure_comms).compare("ssl") == 0) {
        // run with Secure Sockets Layer comms
        std::string path_to_certs(absl::GetFlag(FLAGS_certs));
        expandEnvVariables(path_to_certs);
        std::string root_cert = catena::readFile(path_to_certs + "/ca.crt");
        std::string server_key = catena::readFile(path_to_certs + "/server.key");
        std::string server_cert = catena::readFile(path_to_certs + "/server.crt");
        grpc::SslServerCredentialsOptions ssl_opts(
          absl::GetFlag(FLAGS_mutual_authc) ? GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
                                            : GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);
        ssl_opts.pem_root_certs = root_cert;
        ssl_opts.pem_key_cert_pairs.push_back(
          grpc::SslServerCredentialsOptions::PemKeyCertPair{server_key, server_cert});
        ans = grpc::SslServerCredentials(ssl_opts);

        if (absl::GetFlag(FLAGS_authz)) {
            const std::shared_ptr<grpc::AuthMetadataProcessor> authzProcessor(new JWTAuthMetadataProcessor());
            ans->SetAuthMetadataProcessor(authzProcessor);
        }

    } else if (absl::GetFlag(FLAGS_secure_comms).compare("tls") == 0) {
        std::stringstream why;
        why << "tls support has not been implemented yet, sorry.";
        throw std::runtime_error(why.str());
    } else {
        std::stringstream why;
        why << std::quoted(absl::GetFlag(FLAGS_secure_comms)) << " is not a valid secure_comms option";
        throw std::invalid_argument(why.str());
    }
    return ans;
}



void statusUpdateExample(DeviceModel *dm){
    
    std::thread loop([dm]() {
        // a real service would possibly send status updates, telemetry or audio meters here
        auto a_number = dm->param("/a_number");
        int i = 0;
        dm->valueSetByClient.connect([&i](const ParamAccessor &p, catena::full::ParamIndex idx, const std::string &peer) {
            catena::Value v;
            std::vector<std::string> scopes = {catena::full::kAuthzDisabled};
            p.getValue<false>(&v, idx, scopes);
            std::cout << "Client " << peer << " set " << p.oid() << " to: " << catena::full::printJSON(v) << '\n';
            // a real service would do something with the value here
            if (p.oid() == "/a_number") {
                i = v.int32_value();
            }
        });
        while (globalLoop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            a_number->setValue(i++);
        }
    });
    loop.detach();
}

void RunRPCServer(std::string addr, DeviceModel *dm)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    // get the path to our ssh certificates
    std::string certs_Path(absl::GetFlag(FLAGS_certs));
    expandEnvVariables(certs_Path);

    try {
        // read a json file into a DeviceModel object
        statusUpdateExample(dm);

        // check that static_root is a valid file path
        if (!std::filesystem::exists(absl::GetFlag(FLAGS_static_root))) {
            std::stringstream why;
            why << std::quoted(absl::GetFlag(FLAGS_static_root)) << " is not a valid file path";
            throw std::invalid_argument(why.str());
        }

        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        builder.AddListeningPort(addr, getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        CatenaServiceImpl service(cq.get(), *dm);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms) << '\n';

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

int main(int argc, char* argv[])
{
    DeviceModel *dm;
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
    dm = new DeviceModel(absl::GetFlag(FLAGS_device_model));
  
    std::thread catenaRpcThread(RunRPCServer, addr, dm);
    catenaRpcThread.join();
    delete (dm);
    return 0;
}