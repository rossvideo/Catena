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

// common
#include <IParam.h>

// lite
#include <Device.h>

#include <cassert>

using namespace catena::lite;
using namespace catena::common;

void Device::toProto(::catena::Device& dst, bool shallow) const {
    dst.set_slot(slot_);
    dst.set_detail_level(detail_level_);
    *dst.mutable_default_scope() = default_scope_.toString();
    dst.set_multi_set_enabled(multi_set_enabled_);
    dst.set_subscriptions(subscriptions_);
    if (shallow) { return; }

    // if we're not doing a shallow copy, we need to copy all the Items
    /// @todo: implement deep copies for constraints, menu groups, commands, etc...
    // for now, we can make do with just params
    google::protobuf::Map<std::string, ::catena::Param> dstParams{};
    for (const auto& [name, param] : params_) {
        ::catena::Param dstParam{};
        param->toProto(dstParam);
        dstParams[name] = dstParam;
    }
    dst.mutable_params()->swap(dstParams);
}


