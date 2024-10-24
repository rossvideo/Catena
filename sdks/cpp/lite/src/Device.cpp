/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

// common
#include <IParam.h>

// lite
#include <Device.h>
#include <LanguagePack.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace catena::lite;
using namespace catena::common;

// Initialize the special AUTHZ_DISABLED scope
const std::vector<std::string> Device::kAuthzDisabled = {"__AUTHZ_DISABLED__"};

catena::exception_with_status Device::setValue (const std::string& jptr, catena::Value& src) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = getParam(jptr, ans);
    // we expect this to be a parameter name
    if (param != nullptr) {
        std::string clientScope = "operate"; // temporary until we implement authz
        ans = param->fromProto(src, clientScope);
        valueSetByClient.emit(jptr, param.get(), 0);
    }
    return ans;
}

catena::exception_with_status Device::getValue (const std::string& jptr, catena::Value& dst) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = getParam(jptr, ans);
    
    // we expect this to be a parameter name
    if (param != nullptr) {
        std::string clientScope = "operate"; // temporary until we implement authz
        // we have reached the end of the path, deserialize the value
        param->toProto(dst, clientScope);
    }
    return ans;
}

std::unique_ptr<IParam> Device::getParam(const std::string& fqoid, catena::exception_with_status& status) const {
    // The Path constructor will throw an exception if the json pointer is invalid, so we use a try catch block to catch it.
    try {
        catena::common::Path path(fqoid);
        if (path.empty()) {
            status = catena::exception_with_status("Invalid json pointer", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        if (path.front_is_string()) {
            /**
             * Top level param objects are defined in the device models generated cpp body file and exist for the lifetime of 
             * the program. The device has a map of pointers to these params. These are not unique pointers because the 
             * lifetime of the top level params does not need to be managed.
             */
            IParam* param = getItem<common::ParamTag>(path.front_as_string());
            path.pop();
            if (!param) {
                status = catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT); 
                return nullptr;
            }
            if (path.empty()) {
                /**
                 * Top level params need to be copied into a unique pointer to be returned.
                 * 
                 * This is a shallow copy.
                 */
                return param->copy();
            } else {
                /**
                 * Sub-param objects are created by the getParam function so the lifetime of the object is managed by the caller.
                 */
                return param->getParam(path, status);
            }
        } else {
            status = catena::exception_with_status("Invalid json pointer", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
    } catch (const catena::exception_with_status& why) {
        status = catena::exception_with_status(why.what(), why.status);
        return nullptr;
    }
}

std::unique_ptr<IParam> Device::getCommand(const std::string& fqoid, catena::exception_with_status& status) const {
   // The Path constructor will throw an exception if the json pointer is invalid, so we use a try catch block to catch it.
    try {
        catena::common::Path path(fqoid);
        if (path.empty()) {
            status = catena::exception_with_status("Invalid json pointer", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        if (path.front_is_string()) {
            IParam* param = getItem<common::CommandTag>(path.front_as_string());
            path.pop();
            if (!param) {
                status = catena::exception_with_status("Command not found: " + fqoid, catena::StatusCode::INVALID_ARGUMENT);
                return nullptr;
            }
            if (path.empty()) {
                return param->copy();
            } else {
                status = catena::exception_with_status("sub-commands not implemented", catena::StatusCode::UNIMPLEMENTED);
                return nullptr;
            }
        } else {
            status = catena::exception_with_status("Invalid json pointer", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
    } catch (const catena::exception_with_status& why) {
        status = catena::exception_with_status(why.what(), why.status);
        return nullptr;
    }
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
            param->toProto(dstParam, clientScopes[0]);
            dstParams[name] = dstParam;
        }
    }
    dst.mutable_params()->swap(dstParams); // n.b. lowercase swap, not an error

    // make deep copy of the commands
    google::protobuf::Map<std::string, ::catena::Param> dstCommands{};
    for (const auto& [name, command] : commands_) {
        std::string commandScope = command->getScope();
        if (clientScopes[0] == kAuthzDisabled[0] || std::find(clientScopes.begin(), clientScopes.end(), commandScope) != clientScopes.end()) {
            ::catena::Param dstCommand{};
            command->toProto(dstCommand, clientScopes[0]); // uses param's toProto method
            dstCommands[name] = dstCommand;
        }
    }
    dst.mutable_commands()->swap(dstCommands); // n.b. lowercase swap, not an error

    // make deep copy of the constraints
    google::protobuf::Map<std::string, ::catena::Constraint> dstConstraints{};
    for (const auto& [name, constraint] : constraints_) {
        ::catena::Constraint dstConstraint{};
        constraint->toProto(dstConstraint);
        dstConstraints[name] = dstConstraint;
    }
    dst.mutable_constraints()->swap(dstConstraints); // n.b. lowercase swap, not an error

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


