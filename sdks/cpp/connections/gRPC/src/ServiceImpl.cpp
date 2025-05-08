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

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

using catena::gRPC::CatenaServiceImpl;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Defining the port flag from SharedFlags.h
ABSL_FLAG(uint16_t, port, 6254, "Catena gRPC service port");

CatenaServiceImpl::CatenaServiceImpl(ServerCompletionQueue *cq, IDevice& dm, std::string& EOPath, bool authz)
        : cq_{cq}, 
          dm_{dm}, 
          EOPath_{EOPath}, 
          authorizationEnabled_{authz},
          subscriptionManager_{std::make_unique<catena::common::SubscriptionManager>()} {}

/**
 * Creates the CallData objects for each gRPC command.
 */
void CatenaServiceImpl::init() {
    new catena::gRPC::GetPopulatedSlots(this, dm_, true);
    new catena::gRPC::GetValue(this, dm_, true);
    new catena::gRPC::SetValue(this, dm_, true);
    new catena::gRPC::MultiSetValue(this, dm_, true);
    new catena::gRPC::Connect(this, dm_, true);
    new catena::gRPC::DeviceRequest(this, dm_, true);
    new catena::gRPC::ExternalObjectRequest(this, dm_, true);
    new catena::gRPC::BasicParamInfoRequest(this, dm_, true);
    new catena::gRPC::GetParam(this, dm_, true);
    new catena::gRPC::ExecuteCommand(this, dm_, true);
    new catena::gRPC::AddLanguage(this, dm_, true);
    new catena::gRPC::ListLanguages(this, dm_, true);
    new catena::gRPC::LanguagePackRequest(this, dm_, true);
    new catena::gRPC::UpdateSubscriptions(this, dm_, true);
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> catena::gRPC::Connect::shutdownSignal_;

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
                std::thread(&ICallData::proceed, static_cast<ICallData *>(tag), ok).detach();
                break;
            case ServerCompletionQueue::SHUTDOWN:
                return;
            case ServerCompletionQueue::TIMEOUT:
                break;
        }
    }
}

//Registers current CallData object into the registry
void CatenaServiceImpl::registerItem(ICallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    this->registry_.push_back(RegistryItem(cd));
}

//Deregisters current CallData object from the registry
void CatenaServiceImpl::deregisterItem(ICallData *cd) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = std::find_if(registry_.begin(), registry_.end(),
                            [cd](const RegistryItem &i) { return i.get() == cd; });
    if (it != registry_.end()) {
        registry_.erase(it);
    }
    std::cout << "Active RPCs remaining: " << registry_.size() << '\n';
}
