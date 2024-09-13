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
#include <LanguagePack.h>

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace catena::lite;
using namespace catena::common;

// Initialize the special AUTHZ_DISABLED scope
const std::vector<std::string> Device::kAuthzDisabled = {"__AUTHZ_DISABLED__"};

catena::exception_with_status Device::setValue (const std::string& jptr, catena::Value& src) {
    Path path(jptr);
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    if (path.empty()) {
        return ans;
    }
    Path::Segment top = path.front();
    // we expect this to be a parameter name
    if (std::holds_alternative<std::string>(top)) {
        // get our top-level param descriptor, and a pointer to the value
        IParam* pwv = getItem<common::ParamTag>(std::get<std::string>(top));
        void* ptr = pwv->valuePtr();
        path.pop_front();
        while (!path.empty()) {
            Path::Segment next = path.front();
            if (std::holds_alternative<std::string>(next)) {
                // need to recurse to the next level down
                // this advances both the param descriptor and value pointer
                std::string& oid = std::get<std::string>(next);
                ptr = pwv->valuePtr(ptr, oid);
                pwv = pwv->getParam(oid);
            } else if (std::holds_alternative<Path::Index>(next)) {
                // need to index into the array
                // this only advances the value pointer
                ptr = pwv->valuePtr(ptr, std::get<Path::Index>(next));
            }
            path.pop_front();
        }
        // we have reached the end of the path, deserialize the value
        pwv->fromProto(ptr, src);
        valueSetByClient.emit(jptr, pwv, 0);
    } else {
        std::stringstream ss;
        ss << "Invalid json pointer: " << jptr;
        ss << ", expected first segment to be a string";
        exception_with_status why(ss.str(), catena::StatusCode::INVALID_ARGUMENT);
        ans = std::move(why);
    }
    return ans;
}

catena::exception_with_status Device::getValue (const std::string& jptr, catena::Value& dst) {
    Path path(jptr);
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    if (path.empty()) {
        std::stringstream ss;
        ss << "Invalid json pointer: " << jptr;
        catena::exception_with_status why(ss.str(), catena::StatusCode::NOT_FOUND);
        ans = std::move(why);
        return ans;
    }
    Path::Segment top = path.front();
    // we expect this to be a parameter name
    if (std::holds_alternative<std::string>(top)) {
        // get our top-level param descriptor, and a pointer to the value
        IParam* pwv = getItem<common::ParamTag>(std::get<std::string>(top));
        void* ptr = pwv->valuePtr();
        path.pop_front();
        while (!path.empty()) {
            Path::Segment next = path.front();
            if (std::holds_alternative<std::string>(next)) {
                // need to recurse to the next level down
                // this advances both the param descriptor and value pointer
                std::string& oid = std::get<std::string>(next);
                ptr = pwv->valuePtr(ptr, oid);
                pwv = pwv->getParam(oid);
            } else if (std::holds_alternative<Path::Index>(next)) {
                // need to index into the array
                // this only advances the value pointer
                ptr = pwv->valuePtr(ptr, std::get<Path::Index>(next));
            }
            path.pop_front();
        }
        // we have reached the end of the path, deserialize the value
        pwv->toProto(dst, ptr);
    } else {
        std::stringstream ss;
        ss << "Invalid json pointer: " << jptr;
        ss << ", expected first segment to be a string";
        exception_with_status why(ss.str(), catena::StatusCode::INVALID_ARGUMENT);
        ans = std::move(why);
    }
    return ans;
}

void Device::toProto(::catena::Device& dst, std::vector<std::string>& clientScopes, bool shallow) const {
    dst.set_slot(slot_);
    dst.set_detail_level(detail_level_);
    *dst.mutable_default_scope() = default_scope_.toString();
    dst.set_multi_set_enabled(multi_set_enabled_);
    dst.set_subscriptions(subscriptions_);
    if (shallow) { return; }

    // if we're not doing a shallow copy, we need to copy all the Items
    /// @todo: implement deep copies for constraints, menu groups, commands, etc...
    google::protobuf::Map<std::string, ::catena::Param> dstParams{};
    for (const auto& [name, param] : params_) {
        std::string paramScope = param->getScope();
        if (clientScopes[0] == kAuthzDisabled[0] || std::find(clientScopes.begin(), clientScopes.end(), paramScope) != clientScopes.end()) {
            ::catena::Param dstParam{};
            param->toProto(dstParam);
            dstParams[name] = dstParam;
        }
    }
    dst.mutable_params()->swap(dstParams); // n.b. lowercase swap, not an error

    // make deep copy of the language packs
    ::catena::LanguagePacks dstPacks{};
    for (const auto& [name, pack] : language_packs_) {
        ::catena::LanguagePack dstPack{};
        pack->toProto(dstPack);
        dstPacks.mutable_packs()->insert({name, dstPack});
    }
    dst.mutable_language_packs()->Swap(&dstPacks); // N.B uppercase Swap, not an error

}


void Device::toProto(::catena::LanguagePacks& packs) const {
    packs.clear_packs();
    auto& proto_packs = *packs.mutable_packs();
    for (const auto& [name, pack] : language_packs_) {
        proto_packs[name].set_name(name);
        auto& words = *proto_packs[name].mutable_words();
        words.insert(pack->begin(), pack->end());
    }
}

void Device::toProto(::catena::LanguageList& list) const {
    list.clear_languages();
    for (const auto& [name, pack] : language_packs_) {
        list.add_languages(name);
    }
}


