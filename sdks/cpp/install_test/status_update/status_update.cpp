// Copyright 2024 Ross Video Ltd
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @brief Example program to demonstrate setting up a full Catena service.
 * @file status_update.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 */

// device model
#include "device.status_update.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>

// connections/gRPC
#include <ServiceImpl.h>

// protobuf interface
#include <interface/service.grpc.pb.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

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
#include <functional>
#include "Logger.h"

using grpc::Server;

using namespace catena::common;

// set up the command line parameters
ABSL_FLAG(uint16_t, port, 6254, "Catena service port");
ABSL_FLAG(std::string, certs, "${HOME}/test_certs", "path/to/certs/files");
ABSL_FLAG(std::string, secure_comms, "off", "Specify type of secure comms, options are: \
  \"off\", \"ssl\", \"tls\"");
ABSL_FLAG(bool, mutual_authc, false, "use this to require client to authenticate");
ABSL_FLAG(bool, authz, false, "use OAuth token authorization");
ABSL_FLAG(std::string, static_root, getenv("HOME"), "Specify the directory to search for external objects");

Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
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

void counterUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& counter = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set counter to " << counter;
}

void text_boxUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const std::string& text_box = dynamic_cast<const ParamWithValue<std::string>*>(p)->get();
    DEBUG_LOG << "*** client set text_box to " << text_box;
}

void buttonUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& button = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set button to " << button;
}

void sliderUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& slider = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set slider to " << slider;
}

void combo_boxUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& combo_box = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set combo_box to " << combo_box;
}

void statusUpdateExample(){   
    std::thread loop([]() {
        std::map<std::string, std::function<void(const std::string&, const IParam*, const int32_t)>> handlers;
        handlers["/counter"] = counterUpdateHandler;
        handlers["/text_box"] = text_boxUpdateHandler;
        handlers["/button"] = buttonUpdateHandler;
        handlers["/slider"] = sliderUpdateHandler;
        handlers["/combo_box"] = combo_boxUpdateHandler;

        // this is the "receiving end" of the status update example
        dm.valueSetByClient.connect([&handlers](const std::string& oid, const IParam* p, const int32_t idx) {
            handlers[oid](oid, p, idx);
        });

        catena::exception_with_status err{"", catena::StatusCode::OK};

        // The rest is the "sending end" of the status update example
        std::unique_ptr<IParam> param = dm.getParam("/counter", err);
        if (param == nullptr) {
            throw err;
        }

        // downcast the IParam to a ParamWithValue<int32_t>
        auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param.get());

        while (globalLoop) {
            // update the counter once per second, and emit the event
            std::this_thread::sleep_for(std::chrono::seconds(1));
            {
                std::lock_guard lg(dm.mutex());
                counter.get()++;
                DEBUG_LOG << counter.getOid() << " set to " << counter.get();
                dm.valueSetByServer.emit("/counter", &counter, 0);
            }
        }
    });
    loop.detach();
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // // check that static_root is a valid file path
        // if (!std::filesystem::exists(absl::GetFlag(FLAGS_static_root))) {
        //     std::stringstream why;
        //     why << std::quoted(absl::GetFlag(FLAGS_static_root)) << " is not a valid file path";
        //     throw std::invalid_argument(why.str());
        // }

        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authz = absl::GetFlag(FLAGS_authz);
        CatenaServiceImpl service(cq.get(), dm, EOPath, authz);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        DEBUG_LOG << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        statusUpdateExample();

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[])
{
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    
    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}