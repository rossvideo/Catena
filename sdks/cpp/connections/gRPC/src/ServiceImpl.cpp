/*
 * Copyright 2024 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


// common
#include <Tags.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <GetPopulatedSlots.h>
#include <GetValue.h>
#include <SetValue.h>
#include <MultiSetValue.h>
#include <Connect.h>
#include <DeviceRequest.h>
#include <GetParam.h>
#include <ExternalObjectRequest.h>
#include <BasicParamInfoRequest.h>
#include <ExecuteCommand.h>
#include <AddLanguage.h>
#include <ListLanguages.h>
#include <LanguagePackRequest.h>

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
        std::cout<<"Removed bearer text"<<std::endl;
        grpc::string_ref t = authz->second.substr(7);
        std::string token(t.begin(), t.end());
        auto decoded = jwt::decode(token);
        context->AddProperty("claims", decoded.get_payload());  
    } catch (...) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Invalid bearer token");
    }

    return grpc::Status::OK;
}

/**
 * Returns the current time as a string including microseconds.
 */
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

/**
 * Creates the CallData objects for each gRPC command.
 */
void CatenaServiceImpl::init() {
    
    new GetPopulatedSlots(this, dm_, true);
    new GetValue(this, dm_, true);
    new SetValue(this, dm_, true);
    new MultiSetValue(this, dm_, true);
    new Connect(this, dm_, true);
    new DeviceRequest(this, dm_, true);
    new ExternalObjectRequest(this, dm_, true);
    new BasicParamInfoRequest(this, dm_, true);
    new GetParam(this, dm_, true);
    new ExecuteCommand(this, dm_, true);
    new AddLanguage(this, dm_, true);
    new ListLanguages(this, dm_, true);
    new LanguagePackRequest(this, dm_, true);
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> CatenaServiceImpl::Connect::shutdownSignal_;

// Processes events in the server's completion queue.
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

//Registers current CallData object into the registry
void CatenaServiceImpl::registerItem(CallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    this->registry_.push_back(std::unique_ptr<CallData>(cd));
}

//Deregisters current CallData object from the registry
void CatenaServiceImpl::deregisterItem(CallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = std::find_if(registry_.begin(), registry_.end(),
                            [cd](const RegistryItem &i) { return i.get() == cd; });
    if (it != registry_.end()) {
        registry_.erase(it);
    }
    std::cout << "Active RPCs remaining: " << registry_.size() << '\n';
}


//Gets the scopes from the provided authorization context
std::vector<std::string> CatenaServiceImpl::getScopes(ServerContext &context) {
    if(!authorizationEnabled_){
        // there won't be any scopes if authorization is disabled
        return {};
    }
    auto address =  &context;
    /**
     * If authorization is enabled, get the authorization context as well as
     * the claims from the client_metadata.
     */
    auto contextMeta = &context.client_metadata();
    auto testMeta = *contextMeta;

    //If there is no authorization context, deny the request
    if (contextMeta == nullptr) {
        throw catena::exception_with_status("invalid authorization context", catena::StatusCode::PERMISSION_DENIED);
    }


    bool authenticated = contextMeta->find("authenticated") != contextMeta->end();
    if (!authenticated) {
        throw catena::exception_with_status("Not authenticated", catena::StatusCode::UNAUTHENTICATED);
    }

    // Get the claims from the authorization context
    //std::vector<grpc::string_ref> claimsStr = authContext->FindPropertyValues("claims");
    auto claimsStr = contextMeta->find("claims");
    // If there are no claims, deny the request
    if (claimsStr == contextMeta->end()) {
        throw catena::exception_with_status("No claims found", catena::StatusCode::PERMISSION_DENIED);
    }
    // Parse string of claims into a picojson object
    picojson::value claims;
    // inline std::string picojson::parse(picojson::value &out, const std::string &s)
    std::string err = picojson::parse(claims, claimsStr->second.data());

    // If there was an error parsing the claims, deny the request
    if (!err.empty()) {
        throw catena::exception_with_status("Error parsing claims", catena::StatusCode::PERMISSION_DENIED);
    }

    // Extract the scopes from the claims
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
    /**
     * Freeing the memory associated with claims once we are done with it.
     * Cannot remove the claims from the contextMeta as it is a const object.
     * Be sure to not use this field in the future.
     */

    return scopes;
}
