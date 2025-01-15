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


// common
#include <Tags.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <SetValue.h>
#include <GetValue.h>
#include <GetPopulatedSlots.h>
#include <Connect.h>
#include <DeviceRequest.h>
#include <ExecuteCommand.h>
#include <ExternalObjectRequest.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;


#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

grpc::Status JWTAuthMetadataProcessor::Process(const InputMetadata& auth_metadata, grpc::AuthContext* context, 
                         OutputMetadata* consumed_auth_metadata, OutputMetadata* response_metadata) {
                
    auto authz = auth_metadata.find("authorization");
    if (authz == auth_metadata.end()) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "No bearer token provided");
    } 

    // remove the 'Bearer ' text from the beginning
    try {
        grpc::string_ref t = authz->second.substr(7);
        std::string token(t.begin(), t.end());
        auto decoded = jwt::decode(token);
        context->AddProperty("claims", decoded.get_payload());  
    } catch (...) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Invalid bearer token");
    }

    return grpc::Status::OK;
}

std::string CatenaServiceImpl::timeNow() {
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


CatenaServiceImpl::CatenaServiceImpl(ServerCompletionQueue *cq, Device &dm, std::string& EOPath, bool authz)
        : catena::CatenaService::AsyncService{}, cq_{cq}, dm_{dm}, EOPath_{EOPath}, authorizationEnabled_{authz} {}

void CatenaServiceImpl::init() {
    new GetPopulatedSlots(this, dm_, true);
    new GetValue(this, dm_, true);
    new SetValue(this, dm_, true);
    new Connect(this, dm_, true);
    new DeviceRequest(this, dm_, true);
    new ExternalObjectRequest(this, dm_, true);
    // new GetParam(this, dm_, true);
    new ExecuteCommand(this, dm_, true);
}

vdk::signal<void()> CatenaServiceImpl::Connect::shutdownSignal_;

void CatenaServiceImpl::processEvents() {
    void *tag;
    bool ok;
    std::cout << "Start processing events\n";
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
  
void CatenaServiceImpl::registerItem(CallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    this->registry_.push_back(std::unique_ptr<CallData>(cd));
}

void CatenaServiceImpl::deregisterItem(CallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = std::find_if(registry_.begin(), registry_.end(),
                            [cd](const RegistryItem &i) { return i.get() == cd; });
    if (it != registry_.end()) {
        registry_.erase(it);
    }
    std::cout << "Active RPCs remaining: " << registry_.size() << '\n';
}

std::vector<std::string> CatenaServiceImpl::getScopes(ServerContext &context) {
    if(!authorizationEnabled_){
        // there won't be any scopes if authorization is disabled
        return {};
    }

    auto authContext = context.auth_context();
    if (authContext == nullptr) {
        throw catena::exception_with_status("invalid authorization context", catena::StatusCode::PERMISSION_DENIED);
    }

    std::vector<grpc::string_ref> claimsStr = authContext->FindPropertyValues("claims");
    if (claimsStr.empty()) {
        throw catena::exception_with_status("No claims found", catena::StatusCode::PERMISSION_DENIED);
    }
    // parse string of claims into a picojson object
    picojson::value claims;
    std::string err = picojson::parse(claims, claimsStr[0].data());
    if (!err.empty()) {
        throw catena::exception_with_status("Error parsing claims", catena::StatusCode::PERMISSION_DENIED);
    }

    // extract the scopes from the claims
    std::vector<std::string> scopes;
    const picojson::value::object &obj = claims.get<picojson::object>();
    for (picojson::value::object::const_iterator it = obj.begin(); it != obj.end(); ++it) {
        if (it->first == "scope"){
            std::string scopeClaim = it->second.get<std::string>();
            std::istringstream iss(scopeClaim);
            while (std::getline(iss, scopeClaim, ' ')) {
                scopes.push_back(scopeClaim);
            }
        }
    }
    return scopes;
}




    //  /**
    //  * @brief CallData class for the GetParam RPC
    //  */
    // class GetParam : public CallData {
    //   public:
    //     GetParam(CatenaServiceImpl *service, Device &dm, bool ok)
    //         : service_{service}, dm_{dm}, writer_(&context_),
    //           status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    //         service->registerItem(this);
    //         objectId_ = objectCounter_++;
    //         proceed(service, ok);  // start the process
    //     }
    //     ~GetParam() {}

    //     void proceed(CatenaServiceImpl *service, bool ok) override {
    //         std::cout << "GetParam proceed[" << objectId_ << "]: " << timeNow()
    //                   << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
    //                   << std::endl;

    //         if(!ok){
    //             std::cout << "GetParam[" << objectId_ << "] cancelled\n";
    //             status_ = CallStatus::kFinish;
    //         }
            
    //         switch (status_) {
    //             case CallStatus::kCreate:
    //                 status_ = CallStatus::kProcess;
    //                 service_->RequestGetParam(&context_, &req_, &writer_, service_->cq_, service_->cq_,
    //                                                this);
    //                 break;

    //             case CallStatus::kProcess:
    //                 new GetParam(service_, dm_, ok);  // to serve other clients
    //                 context_.AsyncNotifyWhenDone(this);
    //                 clientScopes_ = getScopes(context_);
    //                 status_ = CallStatus::kWrite;
    //                 // fall thru to start writing

    //             case CallStatus::kWrite:
    //                 try {
    //                     std::cout << "sending param component\n";
    //                     param_ = dm_.param(req_.oid());
    //                     catena::DeviceComponent_ComponentParam ans;
    //                     param_->getParam(&ans, clientScopes_);
                        
    //                     // For now we are sending whole param in one go 
    //                     status_ = CallStatus::kPostWrite;
    //                     writer_.Write(ans, this);
    //                 } catch (catena::exception_with_status &e) {
    //                     status_ = CallStatus::kFinish;
    //                     writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
    //                 } catch (...) {
    //                     status_ = CallStatus::kFinish;
    //                     writer_.Finish(Status::CANCELLED, this);
    //                 }
    //                 break;

    //             case CallStatus::kPostWrite:
    //                 status_ = CallStatus::kFinish;
    //                 writer_.Finish(Status::OK, this);
    //                 break;

    //             case CallStatus::kFinish:
    //                 std::cout << "GetParam[" << objectId_ << "] finished\n";
    //                 service->deregisterItem(this);
    //                 break;
    //         }
    //     }

    //   private:
    //     CatenaServiceImpl *service_;
    //     ServerContext context_;
    //     std::vector<std::string> clientScopes_;
    //     catena::GetParamPayload req_;
    //     catena::PushUpdates res_;
    //     ServerAsyncWriter<catena::DeviceComponent_ComponentParam> writer_;
    //     CallStatus status_;
    //     Device &dm_;
    //     std::unique_ptr<catena::ParamAccessor> param_;
    //     int objectId_;
    //     static int objectCounter_;
    // };
