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


// connections/gRPC
#include <PeerInfo.h>

#include <iostream>
#include <thread>
#include <atomic>

using catena::PeerInfo;
using catena::common::IParam;
//using catena::ParamIndex;

std::atomic<bool> going{}; // flag shared among threads and methods in this translation unit

void PeerInfo::handleValueUpdate(const IParam& param/*, ParamIndex idx*/) {
    if (going) {
        catena::PushUpdates update;
        update.set_slot(0);
        // *update.mutable_value()->mutable_oid() = param.oid();
        // *update.mutable_value()->mutable_value() = param.value<false>();
        writer_->Write(update);
    }
}

grpc::Status PeerInfo::handleConnection() {
    std::thread loop([this]() {
        while ((going = !context_->IsCancelled())) {
            // going will be true until the client disconnects
        }
        // connection is terminated by client so set writer to nullptr
        this->writer_ = nullptr;
    });
    loop.join();
    return grpc::Status::OK;
}