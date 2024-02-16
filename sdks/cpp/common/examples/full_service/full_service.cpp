// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @brief Reads the device.minimal.json device model and provides read/write
 * access via gRPC.
 *
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @file basic_param_access.cpp
 * @copyright Copyright © 2023, Ross Video Ltd
 */

#include <DeviceModel.h>
#include <ParamAccessor.h>
#include <Path.h>
#include <utils.h>
#include <Reflect.h>
#include <PeerManager.h>
#include <PeerInfo.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <service.grpc.pb.h>

#include <jwt-cpp/jwt.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <chrono>

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using catena::CatenaService;

using Index = catena::Path::Index;
using DeviceModel = catena::DeviceModel;
using ParamAccessor = catena::ParamAccessor;


// set up the command line parameters
ABSL_FLAG(uint16_t, port, 6254, "Catena service port");
ABSL_FLAG(std::string, certs, "${HOME}/test_certs", "path/to/certs/files");
ABSL_FLAG(std::string, secure_comms, "off", "Specify type of secure comms, options are: \
  \"off\", \"ssl\", \"tls\"");
ABSL_FLAG(bool, mutual_authc, false, "use this to require client to authenticate");
ABSL_FLAG(bool, authz, false, "use OAuth token authorization");
ABSL_FLAG(std::string, device_model, "../../../example_device_models/device.minimal.json",
          "Specify the JSON device model to use.");

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

// if authz is enabled, inspect the bearer token to decide whether to grant
// access.
//
// right now, it just tests that a token exists, decodes it and prints it out
// much work required to actually validate the token
void authorize(grpc::ServerContext *context /*catena::Param &p*/) {
    if (absl::GetFlag(FLAGS_authz)) {
        auto authz = context->client_metadata();
        auto it = authz.find("authorization");
        if (it == authz.end()) {
            BAD_STATUS("No authorization token found", catena::StatusCode::PERMISSION_DENIED);
        }

        // remove the 'Bearer ' text from the beginning
        grpc::string_ref t = it->second.substr(7);
        std::string token(t.begin(), t.end());
        std::cout << "authz: " << token << '\n';
        auto decoded = jwt::decode(token);
        for (auto &e : decoded.get_payload_json()) {
            std::cout << e.first << ": " << e.second << '\n';
        }
    }
}

class CatenaServiceImpl final : public catena::CatenaService::AsyncService {

    // Status SetValue(ServerContext *context, const ::catena::SetValuePayload *req,
    //                 ::google::protobuf::Empty *res) override {
    //     try {
    //         auto p = dm_.get().param(req->oid());
    //         authorize(context);
    //         p->setValue(context->peer(), req->value(), req->element_index());
    //         std::cout << "SetValue: " << req->oid() << ", " << req->element_index() << '\n';
    //         return Status::OK;

    //     } catch (catena::exception_with_status &why) {
    //         std::cerr << why.what() << std::endl;
    //         grpc::StatusCode gs = static_cast<grpc::StatusCode>(why.status);
    //         return Status(gs, "SetValue failed", why.what());

    //     } catch (...) {
    //         std::cerr << "unhandled exception: " << std::endl;
    //         return Status(grpc::StatusCode::INTERNAL, "SetValue failed");
    //     }
    // }

    // Status DeviceRequest(ServerContext *context, const ::catena::DeviceRequestPayload *req,
    //                      ServerWriter<::catena::DeviceComponent> *writer) override {
    //     try {
    //         std::thread t1(&catena::DeviceModel::streamDevice, &dm_.get(), writer);
    //         t1.join();
    //         return Status::OK;

    //     } catch (catena::exception_with_status &why) {
    //         std::cerr << why.what() << std::endl;
    //         grpc::StatusCode gs = static_cast<grpc::StatusCode>(why.status);
    //         return Status(gs, "DeviceRequest failed", why.what());

    //     } catch (...) {
    //         std::cerr << "unhandled exception: " << std::endl;
    //         return Status(grpc::StatusCode::INTERNAL, "SetValue failed");
    //     }
    // }

    // Status Connect(ServerContext *context, const ::catena::ConnectPayload *req,
    //                ServerWriter<::catena::PushUpdates> *writer) override {
    //     const std::string &peer = context->peer();
    //     auto &peerManager = catena::PeerManager::getInstance();
    //     grpc::Status status;
    //     if (!peerManager.hasPeer(peer)) {
    //         catena::PeerInfo &pi = peerManager.addPeer(context, writer);
    //         status = pi.handleConnection();
    //         peerManager.removePeer(peer);
    //     }
    //     return status;
    // }

  public:
    inline explicit CatenaServiceImpl(ServerCompletionQueue *cq, DeviceModel &dm)
        : catena::CatenaService::AsyncService{}, cq_{cq}, dm_{dm} {}

    void init() { 
        new GetValue(this, dm_); 
        new Connect(this, dm_);
    }

    void processEvents() {
        void *tag;
        bool ok;
        while (cq_->Next(&tag, &ok)) {
            static_cast<CallData *>(tag)->proceed(ok);
        }
    }

  private:
    ServerCompletionQueue *cq_;
    DeviceModel &dm_;

    /**
     * Nested private classes
     */
    enum class CallStatus { kCreate, kProcess, kFinish };
    /**
     * @brief Abstract base class for the CallData classes
     * Provides a the proceed method
     */
    class CallData {
      public:
        virtual void proceed(bool ok) = 0;
    };

    /**
     * @brief CallData class for the GetValue RPC
     */
    class GetValue : public CallData {
      public:
        GetValue(CatenaServiceImpl *service, DeviceModel &dm)
            : service_{service}, dm_{dm}, responder_(&context_), status_{CallStatus::kCreate} {
            proceed(true);
        }

        void proceed(bool ok) override {
            if (status_ == CallStatus::kCreate) {
                status_ = CallStatus::kProcess;
                service_->RequestGetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
            } else if (status_ == CallStatus::kProcess) {
                new GetValue(service_, dm_);
                if (ok) {
                    authorize(&context_);
                    std::unique_ptr<catena::ParamAccessor> param = dm_.param(req_.oid());
                    catena::Value ans;  // oh dear, this is a copy refactoring needed!
                    param->getValue(&ans, req_.element_index());
                    responder_.Finish(ans, Status::OK, this);
                    status_ = CallStatus::kFinish;
                }
            } else {
                GPR_ASSERT(status_ == CallStatus::kFinish);
                delete this;
            }
        }

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::GetValuePayload req_;
        catena::Value res_;
        ServerAsyncResponseWriter<::catena::Value> responder_;
        CallStatus status_;
        DeviceModel &dm_;
    };

    /**
     * @brief CallData class for the GetValue RPC
     */
    class Connect : public CallData {
      public:
        Connect(CatenaServiceImpl *service, DeviceModel &dm)
            : service_{service}, dm_{dm}, writer_(&context_), status_{CallStatus::kCreate} {
            // dm_.valueSetByService.connect([this](const ParamAccessor &p, catena::ParamIndex idx) {
            //     this->res_.mutable_value()->set_oid(p.oid<false>());
            //     p.getValue<false>(this->res_.mutable_value()->mutable_value(), idx);
            //     this->hasUpdate_ = true;
            // });
            std::thread([this](){
                int n = 0;
                while (true) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    this->res_.set_slot(n++);
                    this->hasUpdate_ = true;
                }
            }).detach();
            proceed(true);
        }

        void proceed(bool ok) override {
            if (status_ == CallStatus::kCreate) {
                status_ = CallStatus::kProcess;
                service_->RequestConnect(&context_, &req_, &writer_, service_->cq_, service_->cq_, this);
                context_.AsyncNotifyWhenDone(this);
            } else if (status_ == CallStatus::kProcess) {
                if (context_.IsCancelled()) {
                    new Connect(service_, dm_);
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::CANCELLED, this);
                } else {
                    if (ok) {
                        if (hasUpdate_) {
                            hasUpdate_ = false;
                            authorize(&context_);
                            writer_.Write(res_, this);
                        }
                    } else {
                        // not ok, so we are done
                        status_ = CallStatus::kFinish;
                        writer_.Finish(Status::OK, this); // some other status?
                    }
                }
            } else {
                GPR_ASSERT(status_ == CallStatus::kFinish);
                delete this;
            }
        }

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::ConnectPayload req_;
        catena::PushUpdates res_;
        ServerAsyncWriter<catena::PushUpdates> writer_;
        CallStatus status_;
        DeviceModel &dm_;
        std::atomic<bool> hasUpdate_{false};
    };
};


int main(int argc, char **argv) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    std::string path(absl::GetFlag(FLAGS_certs));
    expandEnvVariables(path);
    std::cout << path << '\n';
    try {
        // read a json file into a DeviceModel object
        DeviceModel dm(absl::GetFlag(FLAGS_device_model));

        // send regular updates to the device model
        std::thread loop([&dm]() {
            auto a_number = dm.param("/a_number");
            int i = 0;
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                a_number->setValue(i++);
            }
        });
        loop.detach();

        // build and run the service asynchronously
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        // build the server
        ServerBuilder builder;
        std::string addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
        builder.AddListeningPort(addr, getServerCredentials());
        std::unique_ptr<ServerCompletionQueue> cq = builder.AddCompletionQueue();
        CatenaServiceImpl service(cq.get(), dm);
        builder.RegisterService(&service);
        std::unique_ptr<Server> server = builder.BuildAndStart();
        std::cout << "Server listening on " << addr
                  << " with secure comms mode: " << absl::GetFlag(FLAGS_secure_comms) << '\n';

        // start processing events
        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // wait for the server to shutdown and tidy up
        server->Wait();
        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}