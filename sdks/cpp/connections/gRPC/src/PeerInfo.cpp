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

#include <connections/gRPC/include/PeerInfo.h>

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