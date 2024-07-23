


#include <connections/gRPC/include/ServiceImpl.h>

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator> 

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


CatenaServiceImpl::CatenaServiceImpl(ServerCompletionQueue *cq, Device &dm, std::string& EOPath)
        : catena::CatenaService::AsyncService{}, cq_{cq}, dm_{dm}, EOPath_{EOPath} {}

void CatenaServiceImpl::init() {
    new GetPopulatedSlots(this, dm_, true);
    new GetValue(this, dm_, true);
    new SetValue(this, dm_, true);
    new Connect(this, dm_, true);
    new DeviceRequest(this, dm_, true);
    new ExternalObjectRequest(this, dm_, true);
    // new GetParam(this, dm_, true);
}

int CatenaServiceImpl::GetPopulatedSlots::objectCounter_ = 0;
int CatenaServiceImpl::GetValue::objectCounter_ = 0; 
int CatenaServiceImpl::SetValue::objectCounter_ = 0;
int CatenaServiceImpl::Connect::objectCounter_ = 0;
int CatenaServiceImpl::DeviceRequest::objectCounter_ = 0;
int CatenaServiceImpl::ExternalObjectRequest::objectCounter_ = 0;

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

// std::vector<std::string> CatenaServiceImpl::getScopes(ServerContext &context) {
//     if (absl::GetFlag(FLAGS_authz) == false) {
//         return {catena::kAuthzDisabled};
//     }

//     auto authContext = context.auth_context();
//     if (authContext == nullptr) {
//         throw catena::exception_with_status("invalid authorization context", catena::StatusCode::PERMISSION_DENIED);
//     }

//     std::vector<grpc::string_ref> claimsStr = authContext->FindPropertyValues("claims");
//     if (claimsStr.empty()) {
//         throw catena::exception_with_status("No claims found", catena::StatusCode::PERMISSION_DENIED);
//     }
//     // parse string of claims into a picojson object
//     picojson::value claims;
//     std::string err = picojson::parse(claims, claimsStr[0].data());
//     if (!err.empty()) {
//         throw catena::exception_with_status("Error parsing claims", catena::StatusCode::PERMISSION_DENIED);
//     }

//     // extract the scopes from the claims
//     std::vector<std::string> scopes;
//     const picojson::value::object &obj = claims.get<picojson::object>();
//     for (picojson::value::object::const_iterator it = obj.begin(); it != obj.end(); ++it) {
//         if (it->first == "scope"){
//             std::string scopeClaim = it->second.get<std::string>();
//             std::istringstream iss(scopeClaim);
//             while (std::getline(iss, scopeClaim, ' ')) {
//                 // check that reserved scope is not used
//                 if (scopeClaim == catena::kAuthzDisabled) {
//                     throw catena::exception_with_status("Invalid scope", catena::StatusCode::PERMISSION_DENIED);
//                 }
//                 scopes.push_back(scopeClaim);
//             }
//         }
//     }
//     return scopes;
// }

CatenaServiceImpl::GetPopulatedSlots::GetPopulatedSlots(CatenaServiceImpl *service, Device &dm, bool ok): service_{service}, dm_{dm}, responder_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service->registerItem(this);
    proceed(service, ok);
}

void CatenaServiceImpl::GetPopulatedSlots::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "GetPopulatedSlots::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;

    if(!ok){
        status_ = CallStatus::kFinish;
    }

    switch(status_){
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetPopulatedSlots(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            {
                new GetPopulatedSlots(service_, dm_, ok);
                context_.AsyncNotifyWhenDone(this);
                catena::SlotList ans;
                ans.add_slots(dm_.slot());
                status_ = CallStatus::kFinish;
                responder_.Finish(ans, Status::OK, this);
            }
        break;

        case CallStatus::kWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kPostWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kFinish:
            std::cout << "GetPopulatedSlots[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
}

CatenaServiceImpl::GetValue::GetValue(CatenaServiceImpl *service, Device &dm, bool ok): service_{service}, dm_{dm}, responder_(&context_),
              status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service->registerItem(this);
    proceed(service, ok);
}

void CatenaServiceImpl::GetValue::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "GetValue::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;

    if(!ok){
        status_ = CallStatus::kFinish;
    }

    switch(status_){
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new GetValue(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            try {
                // std::vector<std::string> clientScopes = getScopes(context_);
                catena::Value ans;
                {
                    Device::LockGuard lg(dm_);
                    catena::lite::IParam* param = dm_.getItem(req_.oid(), Device::ParamTag{});
                    param->toProto(ans);
                }
                status_ = CallStatus::kFinish;
                responder_.Finish(ans, Status::OK, this);
            } catch (catena::exception_with_status &e) {
                status_ = CallStatus::kFinish;
                responder_.FinishWithError(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
            } catch (...) {
                status_ = CallStatus::kFinish;
                responder_.FinishWithError(Status::CANCELLED, this);
            }
        break;

        case CallStatus::kWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kPostWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kFinish:
            std::cout << "GetValue[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
}

CatenaServiceImpl::SetValue::SetValue(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, responder_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service->registerItem(this);
    proceed(service, ok);
}

void CatenaServiceImpl::SetValue::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "SetValue::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestSetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new SetValue(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            try {
                //std::vector<std::string> clientScopes = getScopes(context_);
                auto dstParam = dm_.getItem(req_.oid(), Device::ParamTag{});
                {
                    Device::LockGuard lg(dm_);
                    dstParam->fromProto(req_.value());
                }
                dm_.valueSetByClient.emit(req_.oid(), dstParam, req_.element_index());
                status_ = CallStatus::kFinish;
                responder_.Finish(::google::protobuf::Empty{}, Status::OK, this);
            } catch (catena::exception_with_status &e) {
                errorStatus_ = Status(static_cast<grpc::StatusCode>(e.status), e.what());
                status_ = CallStatus::kFinish;
                responder_.Finish(::google::protobuf::Empty{}, errorStatus_, this);
            } catch (...) {
                errorStatus_ = Status(grpc::StatusCode::INTERNAL, "unknown error");
                status_ = CallStatus::kFinish;
                responder_.Finish(::google::protobuf::Empty{}, errorStatus_, this);
            }
            break;
        
        case CallStatus::kWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kPostWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kFinish:
            std::cout << "SetValue[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
}

CatenaServiceImpl::Connect::Connect(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::Connect::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "Connect proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    // The newest connect object (the one that has not yet been attached to a client request)
    // will send shutdown signal to cancel all open connections
    if(!ok && status_ != CallStatus::kFinish){
        std::cout << "Connect[" << objectId_ << "] cancelled\n";
        std::cout << "Cancelling all open connections" << std::endl;
        shutdownSignal_.emit();
        status_ = CallStatus::kFinish;
    }

    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestConnect(&context_, &req_, &writer_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new Connect(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            shutdownSignalId_ = shutdownSignal_.connect([this](){
                context_.TryCancel();
                hasUpdate_ = true;
                this->cv_.notify_one();
            });
            // pushUpdatesId_ = dm_.pushUpdates.connect([this](const std::string& oid, const IParam& p, const int32_t idx){
            //     try{
            //         if (!this->context_.IsCancelled()){
            //             //std::vector<std::string> scopes = getScopes(this->context_);
            //             this->res_.mutable_value()->set_oid(oid);
            //             this->res_.mutable_value()->set_element_index(idx);
            //             Device::LockGuard lg(dm_);
            //             p.toProto(*this->res_.mutable_value()->mutable_value());
            //         }
            //         this->hasUpdate_ = true;
            //         this->cv_.notify_one();
            //     }catch(catena::exception_with_status& why){
            //         // Error is thrown for connected clients without authorization
            //         // Don't need to send any updates to unauthorized clients
            //     } 
            // });
            valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
                try{
                    if (!this->context_.IsCancelled()){
                        //std::vector<std::string> scopes = getScopes(this->context_);
                        this->res_.mutable_value()->set_oid(oid);
                        this->res_.mutable_value()->set_element_index(idx);
                        Device::LockGuard lg(dm_);
                        p->toProto(*this->res_.mutable_value()->mutable_value());
                    }
                    this->hasUpdate_ = true;
                    this->cv_.notify_one();
                }catch(catena::exception_with_status& why){
                    // Error is thrown for connected clients without authorization
                    // Don't need to send any updates to unauthorized clients
                } 
            });

            // send client a empty update with slot of the device
            {
                catena::PushUpdates populatedSlots;
                populatedSlots.set_slot(dm_.slot());
                writer_.Write(populatedSlots, this);
            }
            status_ = CallStatus::kWrite;
            // fall thru to start writing

        case CallStatus::kWrite:
            lock.lock();
            std::cout << "waiting on cv : " << timeNow() << std::endl;
            cv_.wait(lock, [this] { return hasUpdate_; });
            std::cout << "cv wait over : " << timeNow() << std::endl;
            hasUpdate_ = false;
            if (context_.IsCancelled()) {
                status_ = CallStatus::kFinish;
                std::cout << "Connection[" << objectId_ << "] cancelled\n";
                writer_.Finish(Status::CANCELLED, this);
                break;
            } else {
                std::cout << "sending update\n";
                res_.set_slot(dm_.slot());
                writer_.Write(res_, this);
            }
            lock.unlock();
            break;

        case CallStatus::kPostWrite:
            // not needed
            status_ = CallStatus::kFinish;
            break;

        case CallStatus::kFinish:
            std::cout << "Connect[" << objectId_ << "] finished\n";
            shutdownSignal_.disconnect(shutdownSignalId_);
            dm_.valueSetByClient.disconnect(valueSetByClientId_);
            service->deregisterItem(this);
            break;
    }
}

CatenaServiceImpl::DeviceRequest::DeviceRequest(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::DeviceRequest::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "DeviceRequest proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestDeviceRequest(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                            this);
            break;

        case CallStatus::kProcess:
            new DeviceRequest(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            //clientScopes_ = getScopes(context_);
            // deviceStream_.attachClientScopes(clientScopes_);
            // shutdownSignalId_ = shutdownSignal.connect([this](){
            //     context_.TryCancel();
            //     std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
            // });
            status_ = CallStatus::kWrite;
            // fall thru to start writing

        case CallStatus::kWrite:
            {
                catena::DeviceComponent deviceMessage{};
                catena::Device* dstDevice = deviceMessage.mutable_device();
                {
                    Device::LockGuard lg(dm_);
                    dm_.toProto(*dstDevice, false); // select the deep copy option
                }
                status_ = CallStatus::kPostWrite;
                writer_.Write(deviceMessage, this);
            }
            break;

        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        case CallStatus::kFinish:
            std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
            //shutdownSignal.disconnect(shutdownSignalId_);
            service->deregisterItem(this);
            break;
    }
}

CatenaServiceImpl::ExternalObjectRequest::ExternalObjectRequest(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
    status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::ExternalObjectRequest::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "ExternalObjectRequest proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        std::cout << "ExternalObjectRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }
    
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
            try {
                std::cout << "sending external object " << req_.oid() <<"\n";
                std::string path = service_->EOPath_;
                path.append(req_.oid());

                if (!std::filesystem::exists(path)) {
                    std::cout << "ExternalObjectRequest[" << objectId_ << "] file not found\n";
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

                //For now we are sending the whole file in one go
                std::cout << "ExternalObjectRequest[" << objectId_ << "] sent\n";
                status_ = CallStatus::kPostWrite;
                writer_.Write(obj, this);
            } catch (catena::exception_with_status &e) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
            }
            break;

        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        case CallStatus::kFinish:
            std::cout << "ExternalObjectRequest[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
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
