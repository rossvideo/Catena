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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
#include <IParam.h>
#include <Device.h>
#include <LanguagePack.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>
#include <IMenuGroup.h> 
#include <Menu.h>

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace catena::common;

catena::exception_with_status Device::setValueTry (const std::string& jptr, catena::Value& value, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = getParam(jptr, ans, authz);
        if (param != nullptr) {
            if (!authz.readAuthz(*param)) {
                ans = catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
            }
            else if (!authz.writeAuthz(*param)) {
                ans = catena::exception_with_status("Not authorized to write to param", catena::StatusCode::PERMISSION_DENIED);
            }
            else if (!param->validateSize(value)) {
                ans = catena::exception_with_status("Value exceeds maximum length", catena::StatusCode::OUT_OF_RANGE);
            }
        }
        return ans;
}

catena::exception_with_status Device::multiSetValue (catena::MultiSetValuePayload src, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    std::vector<std::pair<const catena::SetValuePayload*, std::unique_ptr<IParam>>> requests;
    // Go through all setValuePayloads and verify the requests.
    for (auto& setValuePayload : src.values()) {
        auto& oid = setValuePayload.oid();
        requests.push_back(std::make_pair(&setValuePayload, getParam(oid, ans, authz)));
        auto &param = requests.back().second;
        if (param == nullptr) {
            // ans already defined from getParam()
            // Remove the last request from the back in case oid has /-.
            requests.pop_back();
            break;
        }
        else if (!authz.readAuthz(*param)) {
            ans = catena::exception_with_status("Not authorized to read the param " + oid, catena::StatusCode::INVALID_ARGUMENT);
            break;
        }
        else if (!authz.writeAuthz(*param)) {
            ans = catena::exception_with_status("Not authorized to write to param " + oid, catena::StatusCode::PERMISSION_DENIED);
            break;
        }
        else if (!param->validateSize(setValuePayload.value())) {
            ans = catena::exception_with_status("Value exceeds maximum length of " + oid, catena::StatusCode::OUT_OF_RANGE);
            break;
        }
    }
    /** 
     * Since we add empty slots if the oid ends with "/-", we need to
     * delete those empty slots if an issue comes up.
     * We add empty slots above to determine if the param will exceed its limit
     * or not.
     * Alternatively you could implement a size var in the param descriptor.
     */
    if (ans.status != catena::StatusCode::OK) {
        for (auto &[setValuePayload, param] : requests) {
            /**
             * The Path constructor will throw an exception if the json pointer
             * is invalid, so we use a try catch block to catch it.
             */
            try {
                catena::common::Path path(setValuePayload->oid());
                if (path.back_is_index()) {
                    if (path.back_as_index() == catena::common::Path::kEnd) {
                        // Index is "/-". Get parent and remove addition.
                        path.popBack();
                        getParam(path, ans, authz)->popBack(authz, ans);
                    }
                }
            } catch (const catena::exception_with_status& why) {}
        }
    }
    // If there were no issues then commit all set value requests.
    else {
        for (auto &[setValuePayload, param] : requests) {
            ans = param->fromProto(setValuePayload->value(), authz);            
        }
    }
    return ans;
}

catena::exception_with_status Device::setValue (const std::string& jptr, catena::Value& src, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = getParam(jptr, ans, authz);
    if (param != nullptr) {
        ans = param->fromProto(src, authz);
        valueSetByClient.emit(jptr, param.get(), 0);
    }
    return ans;
}

catena::exception_with_status Device::getValue (const std::string& jptr, catena::Value& dst, Authorizer& authz) const {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    /**
     * Converting to Path object to validate input (cant end with /-).
     * The Path constructor will throw an exception if the json pointer is
     * invalid, so we use a try catch block to catch it.
     */
    try {
        catena::common::Path path(jptr);
        if (path.back_is_index()) {
            if (path.back_as_index() == catena::common::Path::kEnd) {
                // Index is "-"
                ans = catena::exception_with_status("Index out of bounds in path " + jptr, catena::StatusCode::INVALID_ARGUMENT);
            }
        }
        if (ans.status == catena::StatusCode::OK) {
            std::unique_ptr<IParam> param = getParam(path, ans, authz);
            // we expect this to be a parameter name
            if (param != nullptr) {
                // we have reached the end of the path, deserialize the value
                ans = param->toProto(dst, authz);
            }
        }
    } catch (const catena::exception_with_status& why) {
        ans = catena::exception_with_status(why.what(), why.status);
    }
    return ans;
}

catena::exception_with_status Device::getLanguagePack(const std::string& languageId, ComponentLanguagePack& pack) const {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    auto foundPack = language_packs_.find(languageId);
    // ERROR: Did not find the pack.
    if (foundPack == language_packs_.end()) {
        return catena::exception_with_status("Language pack '" + languageId + "' not found", catena::StatusCode::NOT_FOUND);
    }
    // Setting the code and transfering language pack info.
    pack.set_language(languageId);
    auto languagePack = pack.mutable_language_pack();
    foundPack->second->toProto(*languagePack);
    // Returning an OK status.
    return catena::exception_with_status("", catena::StatusCode::OK);
}

catena::exception_with_status Device::addLanguage (catena::AddLanguagePayload& language, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    // Admin scope required.
    if (!authz.hasAuthz(Scopes().getForwardMap().at(Scopes_e::kAdmin) + ":w")) {
        return catena::exception_with_status("Not authorized to add language", catena::StatusCode::PERMISSION_DENIED);
    } else {
        auto& name = language.language_pack().name();
        auto& id = language.id();
        // Making sure LanguagePack is properly formatted.
        if (name.empty() || id.empty()) {
            return catena::exception_with_status("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
        }
        // added_packs_ here to maintain ownership in device scope.
        added_packs_[id] = std::make_shared<LanguagePack>(id, name, LanguagePack::ListInitializer{}, *this);
        language_packs_[id]->fromProto(language.language_pack());      
        // Pushing update to connect gRPC.
        ComponentLanguagePack pack;
        ans = getLanguagePack(id, pack);
        languageAddedPushUpdate.emit(pack);
    }
    return ans;
}

std::unique_ptr<IParam> Device::getParam(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) const {
    // The Path constructor will throw an exception if the json pointer is invalid, so we use a try catch block to catch it.
    try {
        catena::common::Path path(fqoid);
        return getParam(path, status, authz);
    } catch (const catena::exception_with_status& why) {
        status = catena::exception_with_status(why.what(), why.status);
        return nullptr;
    }
}

std::unique_ptr<IParam> Device::getParam(catena::common::Path& path, catena::exception_with_status& status, Authorizer& authz) const {
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
        if (!param) {
            status = catena::exception_with_status("Param " + path.fqoid() + " does not exist", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }
        if (!authz.readAuthz(*param)) {
            status = catena::exception_with_status("Not authorized to read the param " + path.fqoid(), catena::StatusCode::PERMISSION_DENIED); 
            return nullptr;
        }
        path.pop();
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
            return param->getParam(path, authz, status);
        }
    } else {
        status = catena::exception_with_status("Invalid json pointer " + path.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
        return nullptr;
    }
}

std::vector<std::unique_ptr<IParam>> Device::getTopLevelParams(catena::exception_with_status& status, Authorizer& authz) const {
    std::vector<std::unique_ptr<IParam>> result;
    try {
        for (const auto& [name, param] : params_) {
            if (authz.readAuthz(*param)) { 
                Path path{name};
                std::cout << "Checking path: '" << path.toString(true) << "'" << std::endl;
                auto param_ptr = getParam(path.toString(true), status, authz);  
                if (param_ptr) {
                    std::cout << "Successfully got param: '" << path.toString(true) << "'" << std::endl;
                    result.push_back(std::move(param_ptr));
                } else {
                    std::cout << "Failed to get param: " << status.what() << std::endl;
                }
            }
        }
    } catch (const catena::exception_with_status& why) {
        status = catena::exception_with_status(why.what(), why.status);
    }
    return result;
}

std::unique_ptr<IParam> Device::getCommand(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) const {
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

void Device::toProto(::catena::Device& dst, Authorizer& authz, bool shallow) const {
    dst.set_slot(slot_);
    dst.set_detail_level(detail_level_);
    *dst.mutable_default_scope() = default_scope_;
    dst.set_multi_set_enabled(multi_set_enabled_);
    dst.set_subscriptions(subscriptions_);
    if (shallow) { return; }

    // if we're not doing a shallow copy, we need to copy all the Items
    google::protobuf::Map<std::string, ::catena::Param> dstParams{};
    for (const auto& [name, param] : params_) {
        if (authz.readAuthz(*param)) {
            ::catena::Param dstParam{};
            param->toProto(dstParam, authz);
            dstParams[name] = dstParam;
        }
    }
    dst.mutable_params()->swap(dstParams); // n.b. lowercase swap, not an error

    // make deep copy of the commands
    google::protobuf::Map<std::string, ::catena::Param> dstCommands{};
    for (const auto& [name, command] : commands_) {
        if (authz.readAuthz(*command)) {
            ::catena::Param dstCommand{};
            command->toProto(dstCommand, authz); // uses param's toProto method
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

    // make a copy of the menu groups
    google::protobuf::Map<std::string, ::catena::MenuGroup> dstMenuGroups{};
    for (const auto& [name, group] : menu_groups_) {
        ::catena::MenuGroup dstGroup{};
        group->toProto(dstGroup, false);
        dstMenuGroups[name] = dstGroup;
    }
    dst.mutable_menu_groups()->swap(dstMenuGroups); // lowercase swap, not an error
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

catena::DeviceComponent Device::DeviceSerializer::getNext() {
    if (hasMore()) {
        handle_.resume();
        handle_.promise().rethrow_if_exception();
    }
    return std::move(handle_.promise().deviceMessage); 
}

Device::DeviceSerializer Device::getComponentSerializer(Authorizer& authz, bool shallow) const {
    catena::DeviceComponent component{};
    if (!shallow) {
        // If not shallow, send the whole device as a single message
        toProto(*component.mutable_device(), authz, shallow);
        co_return component; 
    }

    // If shallow, send the device in parts
    // send the basic device info first
    catena::Device* dst = component.mutable_device();
    dst->set_slot(slot_);
    dst->set_detail_level(detail_level_);
    *dst->mutable_default_scope() = default_scope_;
    dst->set_multi_set_enabled(multi_set_enabled_);
    dst->set_subscriptions(subscriptions_);

    for (auto& scope : access_scopes_) {
        dst->add_access_scopes(scope);
    }

    for (auto& [menu_group_name, menu_group] : menu_groups_) {
        ::catena::MenuGroup* dstMenuGroup = &(*dst->mutable_menu_groups())[menu_group_name];
        menu_group->toProto(*dstMenuGroup, true);
    }

    for (const auto& [language, language_pack] : language_packs_) {
        co_yield component; // yield the previous component before overwriting it
        component.Clear();
        ::catena::LanguagePack* dstPack = component.mutable_language_pack()->mutable_language_pack();
        language_pack->toProto(*dstPack);
        component.mutable_language_pack()->set_language(language);
    }

    for (const auto& [name, constraint] : constraints_) {
        co_yield component; // yield the previous component before overwriting it
        component.Clear();
        ::catena::Constraint* dstConstraint = component.mutable_shared_constraint()->mutable_constraint();
        constraint->toProto(*dstConstraint);
        component.mutable_shared_constraint()->set_oid(name);
    }

    for (const auto& [name, param] : params_) {
        if (authz.readAuthz(*param)) {
            co_yield component; // yield the previous component before overwriting it
            component.Clear();
            ::catena::Param* dstParam = component.mutable_param()->mutable_param();
            param->toProto(*dstParam, authz);
            component.mutable_param()->set_oid(name);
        }
    }

    for (const auto& [name, command] : commands_) {
        if (authz.readAuthz(*command)) {
            co_yield component; // yield the previous component before overwriting it
            component.Clear();
            ::catena::Param* dstCommand = component.mutable_command()->mutable_param();
            command->toProto(*dstCommand,authz);
            component.mutable_command()->set_oid(name);
        }
    }

    for (const auto& [name, menu_groups_] : menu_groups_) {
        for (const auto& [menu_name, menu] : *menu_groups_->menus()) {
            co_yield component; // yield the previous component before overwriting it
            component.Clear();
            ::catena::Menu* dstMenu = component.mutable_menu()->mutable_menu();
            menu.toProto(*dstMenu);
            std::string oid = "/" + name + "/" + menu_name;
            component.mutable_menu()->set_oid(oid);
        }
    }
    
    // return the last component
    co_return component;
}

