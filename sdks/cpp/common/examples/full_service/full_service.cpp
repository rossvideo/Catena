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
#include <signal.h>
#include <condition_variable>
#include <filesystem>

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
ABSL_FLAG(std::string, static_root, "${HOME}", "Specify the directory to search for external objects");

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
/**
 * @brief Implements the Catena Service
 */
class CatenaServiceImpl final : public catena::CatenaService::AsyncService {
  public:
    inline explicit CatenaServiceImpl(ServerCompletionQueue *cq, DeviceModel &dm)
        : catena::CatenaService::AsyncService{}, cq_{cq}, dm_{dm} {}

    void inline init() {
        new GetValue(this, dm_, true);
        new SetValue(this, dm_, true);
        new Connect(this, dm_, true);
        new DeviceRequest(this, dm_, true);
        new ExternalObjectRequest(this, dm_, true);
    }

    void processEvents() {
        void *tag;
        bool ok;
        while (true) {
            gpr_timespec deadline =
              gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_seconds(1, GPR_TIMESPAN));
            switch (cq_->AsyncNext(&tag, &ok, deadline)) {
                case ServerCompletionQueue::GOT_EVENT:
                    std::thread(&CallData::proceed, static_cast<CallData *>(tag), this, ok).detach();
                    break;
                case ServerCompletionQueue::SHUTDOWN:
                    return;
                case ServerCompletionQueue::TIMEOUT:
                    break;
            }
        }
    }

  private:
    class CallData;
    using Registry = std::vector<std::unique_ptr<CatenaServiceImpl::CallData>>;
    using RegistryItem = std::unique_ptr<CatenaServiceImpl::CallData>;
    Registry registry_;

    ServerCompletionQueue *cq_;
    DeviceModel &dm_;
  
    void registerItem(CallData *cd) {
        this->registry_.push_back(std::unique_ptr<CallData>(cd));
    }

    void deregisterItem(CallData *cd) {
        auto it = std::find_if(registry_.begin(), registry_.end(),
                               [cd](const RegistryItem &i) { return i.get() == cd; });
        if (it != registry_.end()) {
            registry_.erase(it);
        }
    }

    /**
     * Nested private classes
     */
    enum class CallStatus { kCreate, kProcess, kWrite, kPostWrite, kFinish };
    /**
     * @brief Abstract base class for the CallData classes
     * Provides a the proceed method
     */
    class CallData {
      public:
        virtual void proceed(CatenaServiceImpl *service, bool ok) = 0;
        virtual ~CallData() {}
    };
    
    /**
     * @brief CallData class for the GetValue RPC
     */
    class GetValue : public CallData {
      public:
        GetValue(CatenaServiceImpl *service, DeviceModel &dm, bool ok)
            : service_{service}, dm_{dm}, responder_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
            objectId_ = objectCounter_++;
            service->registerItem(this);
            proceed(service, ok);
        }

        void proceed(CatenaServiceImpl *service, bool ok) override {
            std::cout << "GetValue::proceed[" << objectId_ << "]: " << timeNow()
                      << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok << '\n';
            switch(status_){
                case CallStatus::kCreate:
                    status_ = CallStatus::kProcess;
                    service_->RequestGetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
                    break;

                case CallStatus::kProcess:
                    new GetValue(service_, dm_, ok);
                    if (ok) {
                        context_.AsyncNotifyWhenDone(this);
                        if (validateRequest_()) {
                            try {
                                authorize(&context_);
                                std::unique_ptr<catena::ParamAccessor> param = dm_.param(req_.oid());
                                catena::Value ans;  // oh dear, this is a copy refactoring needed!
                                param->getValue(&ans, req_.element_index());
                                responder_.Finish(ans, Status::OK, this);
                                status_ = CallStatus::kFinish;
                            } catch (catena::exception_with_status &e) {
                                responder_.FinishWithError(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
                                status_ = CallStatus::kFinish;
                            } catch (...) {
                                responder_.FinishWithError(Status::CANCELLED, this);
                                status_ = CallStatus::kFinish;
                            }
                        } else {
                            status_ = CallStatus::kFinish;
                            responder_.FinishWithError(Status::CANCELLED, this);
                        }
                    }
                    break;

                case CallStatus::kWrite:
                    // not needed
                    break;

                case CallStatus::kPostWrite:
                    status_ = CallStatus::kFinish;
                    break;

                case CallStatus::kFinish:
                    std::cout << "GetValue[" << objectId_ << "] finished\n";
                    service->deregisterItem(this);
                    break;
            }
        }

      private:
        bool validateRequest_() {
            bool valid = !req_.oid().empty();
            if (!valid) {
                std::cout << "GetValue[" << objectId_ << "]Ignoring invalid request: oid is empty\n";
            }
            return valid;
        }

        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::GetValuePayload req_;
        catena::Value res_;
        ServerAsyncResponseWriter<::catena::Value> responder_;
        CallStatus status_;
        DeviceModel &dm_;
        int objectId_;
        static int objectCounter_;
    };

    /**
     * @brief CallData class for the SetValue RPC
     */
    class SetValue : public CallData {
      public:
        SetValue(CatenaServiceImpl *service, DeviceModel &dm, bool ok)
            : service_{service}, dm_{dm}, responder_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
            objectId_ = objectCounter_++;
            service->registerItem(this);
            proceed(service, ok);
        }

        void proceed(CatenaServiceImpl *service, bool ok) override {
            std::cout << "SetValue::proceed[" << objectId_ << "]: " << timeNow()
                      << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok << '\n';
            switch (status_) {
                case CallStatus::kCreate:
                    status_ = CallStatus::kProcess;
                    service_->RequestSetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
                    break;

                case CallStatus::kProcess:
                    new SetValue(service_, dm_, ok);
                    if (ok) {
                        context_.AsyncNotifyWhenDone(this);
                        if (validateRequest_()) {
                            try {
                                authorize(&context_);
                                std::unique_ptr<catena::ParamAccessor> param = dm_.param(req_.oid());
                                param->setValue(context_.peer(), req_.value(), req_.element_index());
                                responder_.Finish(::google::protobuf::Empty{}, Status::OK, this);
                                status_ = CallStatus::kFinish;
                            } catch (catena::exception_with_status &e) {
                                errorStatus_ = Status(static_cast<grpc::StatusCode>(e.status), e.what());
                                status_ = CallStatus::kFinish;
                                responder_.Finish(::google::protobuf::Empty{}, errorStatus_, this);
                            } catch (...) {
                                errorStatus_ = Status(grpc::StatusCode::INTERNAL, "unknown error");
                                status_ = CallStatus::kFinish;
                                responder_.Finish(::google::protobuf::Empty{}, errorStatus_, this);
                            }
                            
                        } else {
                            status_ = CallStatus::kFinish;
                            responder_.FinishWithError(Status::CANCELLED, this);
                        }
                    }
                    break;
                
                case CallStatus::kWrite:
                    // not needed
                    break;

                case CallStatus::kPostWrite:
                    status_ = CallStatus::kFinish;
                    break;

                case CallStatus::kFinish:
                    std::cout << "SetValue[" << objectId_ << "] finished\n";
                    service->deregisterItem(this);
                    break;
            }
        }

      private:
        bool validateRequest_() {
            bool valid = !req_.oid().empty();
            if (!valid) {
                std::cout << "SetValue[" << objectId_ << "] Ignoring invalid request: oid is empty\n";
            }
            return valid;
        }

        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::SetValuePayload req_;
        catena::Value res_;
        ServerAsyncResponseWriter<::google::protobuf::Empty> responder_;
        CallStatus status_;
        DeviceModel &dm_;
        Status errorStatus_;
        int objectId_;
        static int objectCounter_;
    };

    /**
     * @brief CallData class for the Connect RPC
     */
    class Connect : public CallData {
      public:
        Connect(CatenaServiceImpl *service, DeviceModel &dm, bool ok)
            : service_{service}, dm_{dm}, writer_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
            service->registerItem(this);
            objectId_ = objectCounter_++;
            if (ok) {
                connectId_ = dm_.valueSetByService.connect([this](const ParamAccessor &p, catena::ParamIndex idx) {
                    std::unique_lock<std::mutex> lock(this->mtx_);
                    this->res_.mutable_value()->set_oid(p.oid());
                    p.getValue<false>(this->res_.mutable_value()->mutable_value(), idx);
                    this->hasUpdate_ = true;
                    lock.unlock();
                    this->cv_.notify_one();
                });
            }
            proceed(service, ok);  // start the process
        }
        ~Connect() {}

        void proceed(CatenaServiceImpl *service, bool ok) override {
            std::cout << "Connect proceed[" << objectId_ << "] " << timeNow()
                      << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                      << std::endl;
            switch (status_) {
                case CallStatus::kCreate:
                    status_ = CallStatus::kProcess;
                    service_->RequestConnect(&context_, &req_, &writer_, service_->cq_, service_->cq_, this);
                    break;

                case CallStatus::kProcess:
                    new Connect(service_, dm_, ok);  // to serve other clients
                    context_.AsyncNotifyWhenDone(this);
                    status_ = CallStatus::kWrite;
                    // fall thru to start writing

                case CallStatus::kWrite:
                    if (ok) {
                        std::unique_lock<std::mutex> lock(this->mtx_);
                        std::cout << "waiting on cv : " << timeNow() << std::endl;
                        cv_.wait(lock, [this] { return hasUpdate_; });
                        std::cout << "cv wait over : " << timeNow() << std::endl;
                        hasUpdate_ = false;
                        lock.unlock();
                        authorize(&context_);
                        std::cout << "sending update\n";
                        if (context_.IsCancelled()) {
                            std::cout << "Connect[" << objectId_ << "] cancelled\n";
                            status_ = CallStatus::kFinish;
                            dm_.valueSetByService.disconnect(connectId_);
                            service->deregisterItem(this);
                            break;
                        } else {
                            writer_.Write(res_, this);
                        }
                    } else {
                        std::cout << "Server shutting down: connect[" << objectId_ << "] cancelled\n";
                        status_ = CallStatus::kFinish;
                        dm_.valueSetByService.disconnect(connectId_);
                        service->deregisterItem(this);
                    }
                    break;

                case CallStatus::kPostWrite:
                    writer_.Finish(Status::OK, this);
                    status_ = CallStatus::kFinish;
                    dm_.valueSetByService.disconnect(connectId_);
                    service->deregisterItem(this);
                    break;

                case CallStatus::kFinish:
                    std::cout << "Connect[" << objectId_ << "] finished\n";
                    dm_.valueSetByService.disconnect(connectId_);
                    service->deregisterItem(this);
                    break;
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
        std::mutex mtx_;
        std::condition_variable cv_;
        bool hasUpdate_{false};
        int objectId_;
        static int objectCounter_;
        unsigned int connectId_;
    };

    /**
     * @brief CallData class for the DeviceRequest RPC
     * @todo validate the request and pass it to the device model's streamDevice method
     */
    class DeviceRequest : public CallData {
      public:
        DeviceRequest(CatenaServiceImpl *service, DeviceModel &dm, bool ok)
            : service_{service}, dm_{dm}, writer_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
            service->registerItem(this);
            objectId_ = objectCounter_++;
            proceed(service, ok);  // start the process
        }
        ~DeviceRequest() {}

        void proceed(CatenaServiceImpl *service, bool ok) override {
            std::cout << "DeviceRequest proceed[" << objectId_ << "] " << timeNow()
                      << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                      << std::endl;
            switch (status_) {
                case CallStatus::kCreate:
                    status_ = CallStatus::kProcess;
                    service_->RequestDeviceRequest(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                                   this);
                    break;

                case CallStatus::kProcess:
                    new DeviceRequest(service_, dm_, ok);  // to serve other clients
                    context_.AsyncNotifyWhenDone(this);
                    status_ = CallStatus::kWrite;
                    // fall thru to start writing

                case CallStatus::kWrite:
                    if (ok) {
                        authorize(&context_);
                        std::cout << "sending device\n";
                        bool sendComplete = dm_.streamDevice(&writer_, this);
                        status_ = sendComplete ? CallStatus::kPostWrite : CallStatus::kWrite;
                        writer_.Finish(Status::OK, this);
                    } else {
                        status_ = CallStatus::kFinish;
                    }
                    break;

                case CallStatus::kPostWrite:
                    status_ = CallStatus::kFinish;
                    break;

                case CallStatus::kFinish:
                    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
                    service->deregisterItem(this);
                    break;
            }
        }

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::DeviceRequestPayload req_;
        catena::PushUpdates res_;
        ServerAsyncWriter<catena::DeviceComponent> writer_;
        CallStatus status_;
        DeviceModel &dm_;
        int objectId_;
        static int objectCounter_;
    };

    class ExternalObjectRequest : public CallData {
      public:
        ExternalObjectRequest(CatenaServiceImpl *service, DeviceModel &dm, bool ok)
            : service_{service}, dm_{dm}, writer_(&context_),
            status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
            service->registerItem(this);
            objectId_ = objectCounter_++;
            proceed(service, ok);  // start the process
        }
        ~ExternalObjectRequest() {}

        void proceed(CatenaServiceImpl *service, bool ok) override {
            std::cout << "ExternalObjectRequest proceed[" << objectId_ << "] " << timeNow()
                      << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                      << std::endl;
            switch (status_) {
                case CallStatus::kCreate:
                    status_ = CallStatus::kProcess;
                    service_->RequestExternalObjectRequest(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                                   this);
                    break;

                case CallStatus::kProcess:
                    new ExternalObjectRequest(service_, dm_, ok);  // to serve other clients
                    context_.AsyncNotifyWhenDone(this);
                    status_ = CallStatus::kWrite;
                    // fall thru to start writing

                case CallStatus::kWrite:
                    if (ok) {
                        try {
                            authorize(&context_);
                            std::cout << "sending external object " << req_.oid() <<"\n";
                            std::string path = absl::GetFlag(FLAGS_static_root);
                            path.append(req_.oid());

                            if (!std::filesystem::exists(path)) {
                                if(req_.oid()[0] != '/'){
                                    std::stringstream why;
                                    why << __PRETTY_FUNCTION__ << "\nfile '" << req_.oid() << "' not found. HINT: Make sure oid starts with '/' prefix.";
                                    throw catena::exception_with_status(why.str(), catena::StatusCode::NOT_FOUND);
                                }else{
                                    std::stringstream why;
                                    why << __PRETTY_FUNCTION__ << "\nfile '" << req_.oid() << "' not found";
                                    throw catena::exception_with_status(why.str(), catena::StatusCode::NOT_FOUND);
                                }
                            }
                            // read the file into a byte array
                            std::ifstream file(path, std::ios::binary);
                            std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                            
                            catena::ExternalObjectPayload obj;
                            obj.mutable_payload()->set_payload(file_data.data(), file_data.size());
                            writer_.Write(obj, this);

                            //For now we are sending the whole file in one go
                            std::cout << "ExternalObjectRequest[" << objectId_ << "] sent\n";
                            status_ = CallStatus::kPostWrite;
                        } catch (catena::exception_with_status &e) {
                            writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
                            status_ = CallStatus::kFinish;
                        } catch (...) {
                            writer_.Finish(Status::CANCELLED, this);
                            status_ = CallStatus::kFinish;
                        }
                    } else {
                        status_ = CallStatus::kFinish;
                    }
                    break;

                case CallStatus::kPostWrite:
                    writer_.Finish(Status::OK, this);
                    status_ = CallStatus::kFinish;
                    break;

                case CallStatus::kFinish:
                    service->deregisterItem(this);
                    break;
            }
        }

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::ExternalObjectRequestPayload req_;
        catena::PushUpdates res_;
        ServerAsyncWriter<catena::ExternalObjectPayload> writer_;
        CallStatus status_;
        DeviceModel &dm_;
        int objectId_;
        static int objectCounter_;
    };
};

int CatenaServiceImpl::GetValue::objectCounter_ = 0;
int CatenaServiceImpl::Connect::objectCounter_ = 0;
int CatenaServiceImpl::SetValue::objectCounter_ = 0;
int CatenaServiceImpl::DeviceRequest::objectCounter_ = 0;
int CatenaServiceImpl::ExternalObjectRequest::objectCounter_ = 0;


void statusUpdateExample(DeviceModel *dm)
{
    dm->valueSetByClient.connect([](const ParamAccessor &p, catena::ParamIndex idx, const std::string &peer) {
        catena::Value v;
        p.getValue<false>(&v, idx);
        std::cout << "Client " << peer << " set " << p.oid() << " to: " << printJSON(v) << '\n';
        // a real service would do something with the value here
    });
    std::thread loop([dm]() {
        // a real service would possibly send status updates, telemetry or audio meters here
        auto a_number = dm->param("/a_number");
        int i = 0;
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

        ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        builder.AddListeningPort(addr, getServerCredentials());
        std::unique_ptr<ServerCompletionQueue> cq = builder.AddCompletionQueue();
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
