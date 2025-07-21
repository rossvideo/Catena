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

// connections/gRPC
#include <ServiceImpl.h>
#include <Logger.h>
using catena::gRPC::CatenaServiceImpl;

// Defining the port flag from SharedFlags.h
ABSL_FLAG(uint16_t, port, 6254, "Catena gRPC service port");

CatenaServiceImpl::CatenaServiceImpl(ServerCompletionQueue *cq, std::vector<IDevice*> dms, std::string& EOPath, bool authz, uint32_t maxConnections)
    : cq_{cq},
      EOPath_{EOPath}, 
      authorizationEnabled_{authz},
      maxConnections_{maxConnections} {
    // Adding dms to slotMap.
    for (auto dm : dms) {
        if (dms_.contains(dm->slot())) {
            throw std::runtime_error("Device with slot " + std::to_string(dm->slot()) + " already exists in the map.");
        } else {
            dms_[dm->slot()] = dm;
        }
    }
}

/**
 * Creates the CallData objects for each gRPC command.
 */
void CatenaServiceImpl::init() {
    new catena::gRPC::GetPopulatedSlots(this, dms_, true);
    new catena::gRPC::GetValue(this, dms_, true);
    new catena::gRPC::SetValue(this, dms_, true);
    new catena::gRPC::MultiSetValue(this, dms_, true);
    new catena::gRPC::Connect(this, dms_, true);
    new catena::gRPC::DeviceRequest(this, dms_, true);
    new catena::gRPC::ExternalObjectRequest(this, dms_, true);
    new catena::gRPC::ParamInfoRequest(this, dms_, true);
    new catena::gRPC::GetParam(this, dms_, true);
    new catena::gRPC::ExecuteCommand(this, dms_, true);
    new catena::gRPC::AddLanguage(this, dms_, true);
    new catena::gRPC::ListLanguages(this, dms_, true);
    new catena::gRPC::LanguagePackRequest(this, dms_, true);
    new catena::gRPC::UpdateSubscriptions(this, dms_, true);
}

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> catena::gRPC::Connect::shutdownSignal_;

// Processes events in the server's completion queue.
void CatenaServiceImpl::processEvents() {
    void *tag;
    bool ok;
    DEBUG_LOG << "Start processing events\n";
    while (true) {
        gpr_timespec deadline =
            gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_seconds(1, GPR_TIMESPAN));
        switch (cq_->AsyncNext(&tag, &ok, deadline)) {
            case ServerCompletionQueue::GOT_EVENT:
                std::thread(&ICallData::proceed, static_cast<ICallData *>(tag), ok).detach();
                break;
            case ServerCompletionQueue::SHUTDOWN:
                // Waiting until all events are processed to exit.
                while (registry_.size() > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                return;
            case ServerCompletionQueue::TIMEOUT:
                break;
        }
    }
}

bool CatenaServiceImpl::registerConnection(catena::common::IConnect* cd) {
    bool canAdd = false;
    std::lock_guard<std::mutex> lock(connectionMutex_);
    // Find the index to insert the new connection based on priority.
    auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
        [cd](const catena::common::IConnect* connection) { return *cd < *connection; });
    // Based on the iterator, determine if we can add the connection.
    if (connectionQueue_.size() >= maxConnections_) {
        if (it != connectionQueue_.begin()) {
            // Forcefully shutting down lowest priority connection.
            connectionQueue_.front()->shutdown();
            canAdd = true;
        }
    } else {
        canAdd = true;
    }
    // Adding the connection if possible.
    if (canAdd) {
        connectionQueue_.insert(it, cd);
    }
    return canAdd;
}

void CatenaServiceImpl::deregisterConnection(catena::common::IConnect* cd) {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
                           [cd](const catena::common::IConnect* i) { return i == cd; });
    if (it != connectionQueue_.end()) {
        connectionQueue_.erase(it);
    }
    DEBUG_LOG << "Connected users remaining: " << connectionQueue_.size() << '\n';
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
    DEBUG_LOG << "Active RPCs remaining: " << registry_.size() << '\n';
}
