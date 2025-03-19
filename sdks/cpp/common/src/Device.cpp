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

bool Device::tryMultiSetValue (catena::MultiSetValuePayload src, catena::exception_with_status& ans, Authorizer& authz) {
    // Making sure multi set is enabled.
    if (src.values_size() > 1 && !multi_set_enabled_) {
        ans = catena::exception_with_status("Multi-set is disabled for the device in slot " + slot_, catena::StatusCode::PERMISSION_DENIED);
    } else {
        // Looping through and validating set value requests.
        for (const catena::SetValuePayload& setValuePayload : src.values()) {
            try {
                // Getting param, or parent param if the final segment is an index.
                Path path(setValuePayload.oid());
                Path::Index index = Path::kNone;
                if (path.back_is_index()) {
                    index = path.back_as_index();
                    path.popBack();
                }
                std::unique_ptr<IParam> param = getParam(path, ans, authz);
                // Validating serValue operation.
                if (!param) {
                    break;
                } else if (!param->validateSetValue(setValuePayload.value(), index, authz, ans)) {
                    break;
                }
            } catch (const catena::exception_with_status& why) {
                ans = catena::exception_with_status(why.what(), why.status);
                break;
            }
        }
        // Resetting trackers regardless of whether something went wrong or not.
        for (const catena::SetValuePayload& setValuePayload : src.values()) {
            // Creating another exception_with_status as to not overwrite ans.
            catena::exception_with_status status{"", catena::StatusCode::OK};
            // Resetting param's trackers, or parent param's if back is index.
            Path path(setValuePayload.oid());
            if (path.back_is_index()) { path.popBack(); }
            std::unique_ptr<IParam> param = getParam(path, status, authz);
            if (param) { param->resetValidate(); }
        }
    }
    // Returning true if successful.
    return ans.status == catena::StatusCode::OK;
}

catena::exception_with_status Device::commitMultiSetValue (catena::MultiSetValuePayload src, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    // Looping through and commiting all setValue operations.
    for (const catena::SetValuePayload& setValuePayload : src.values()) {
        try {
            // Figuring out parent param for appends and resetValidate().
            Path path(setValuePayload.oid());
            std::unique_ptr<IParam> parent = nullptr;
            std::unique_ptr<IParam> param = nullptr;
            // Get parent.
            if (path.back_is_index()) {
                Path::Index index = path.back_as_index();
                path.popBack();
                parent = getParam(path, ans, authz);
                if (index == Path::kEnd) {
                    param = parent->addBack(authz, ans);
                } else {
                    path.push_back(index);
                    param = parent->getParam(path, authz, ans);
                }
            // Get normal.
            } else {
                param = getParam(path, ans, authz);
            }
            // Setting value and emitting signal.
            ans = param->fromProto(setValuePayload.value(), authz);
            valueSetByClient.emit(setValuePayload.oid(), param.get(), 0);
            // Resetting trackers to match new value.
            if (parent) { parent->resetValidate();
            } else { param->resetValidate(); }

        } catch (const catena::exception_with_status& why) {
            ans = catena::exception_with_status(why.what(), why.status);
            break;
        }
    }
    return ans;
}

catena::exception_with_status Device::setValue (const std::string& jptr, catena::Value& src, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    catena::MultiSetValuePayload setValues;
    catena::SetValuePayload* setValuePayload = setValues.add_values();
    setValuePayload->set_oid(jptr);
    setValuePayload->mutable_value()->CopyFrom(src);
    if (tryMultiSetValue(setValues, ans, authz)) {
        ans = commitMultiSetValue(setValues, authz);
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
                ans = catena::exception_with_status("Index out of bounds in path " + jptr, catena::StatusCode::OUT_OF_RANGE);
            }
        } else {
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
        return catena::exception_with_status("Not authorized to add language ", catena::StatusCode::PERMISSION_DENIED);
    } else {
        auto& name = language.language_pack().name();
        auto& id = language.id();
        // Making sure LanguagePack is properly formatted.
        if (name.empty() || id.empty()) {
            return catena::exception_with_status("Invalid language pack ", catena::StatusCode::INVALID_ARGUMENT);
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
        status = catena::exception_with_status("Invalid json pointer ", catena::StatusCode::INVALID_ARGUMENT);
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
            status = catena::exception_with_status("Param " + path.fqoid() + " does not exist ", catena::StatusCode::NOT_FOUND);
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
                auto param_ptr = getParam(path.toString(true), status, authz);  
                if (param_ptr) {
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
            status = catena::exception_with_status("Invalid json pointer ", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        if (path.front_is_string()) {
            IParam* param = getItem<common::CommandTag>(path.front_as_string());
            path.pop();
            if (!param) {
                status = catena::exception_with_status("Command not found: " + fqoid, catena::StatusCode::NOT_FOUND);
                return nullptr;
            }
            if (path.empty()) {
                return param->copy();
            } else {
                status = catena::exception_with_status("sub-commands not implemented ", catena::StatusCode::UNIMPLEMENTED);
                return nullptr;
            }
        } else {
            status = catena::exception_with_status("Invalid json pointer ", catena::StatusCode::INVALID_ARGUMENT);
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

    // Helper function to serialize a collection of IParams
    auto serializeParamCollection = [&](const std::unordered_map<std::string, IParam*>& collection, 
                                      google::protobuf::Map<std::string, ::catena::Param>& dstMap) {
        for (const auto& [name, param] : collection) {
            if (shouldSendParam(*param, true, authz)) {
                ::catena::Param dstParam{};
                param->toProto(dstParam, authz);
                dstMap[name] = dstParam;
            }
        }
    };

    // Serialize parameters
    google::protobuf::Map<std::string, ::catena::Param> dstParams{};
    serializeParamCollection(params_, dstParams);
    dst.mutable_params()->swap(dstParams); //lowercase swap, not an error

    // Serialize commands
    google::protobuf::Map<std::string, ::catena::Param> dstCommands{};
    serializeParamCollection(commands_, dstCommands);
    dst.mutable_commands()->swap(dstCommands); // n.b. lowercase swap, not an error

    // If in minimal detail level, skip the rest of the serialization
    if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
        return;
    }

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
    // Call the overloaded version with an explicitly constructed empty vector
    static const std::vector<std::string> empty_vector;
    return getComponentSerializer(authz, empty_vector, shallow);
}

Device::DeviceSerializer Device::getComponentSerializer(Authorizer& authz, const std::vector<std::string>& subscribed_oids, bool shallow) const {
    std::cout << "getComponentSerializer received vector size: " << subscribed_oids.size() << std::endl;
    catena::DeviceComponent component{};
    
    // Send basic device information first
    catena::Device* dst = component.mutable_device();
    dst->set_slot(slot_);
    dst->set_detail_level(detail_level_);
    *dst->mutable_default_scope() = default_scope_;
    dst->set_multi_set_enabled(multi_set_enabled_);
    dst->set_subscriptions(subscriptions_);
    for (auto& scope : access_scopes_) {
        dst->add_access_scopes(scope);
    }

    // If detail level is NONE, only send the above device info and return
    if (detail_level_ == catena::Device_DetailLevel_NONE) {
        co_return component;
    }

    // Helper function to check if an OID is subscribed
    auto is_subscribed = [&subscribed_oids, this](const std::string& param_name) {
        // If subscriptions are disabled or we're not in SUBSCRIPTIONS mode, everything is subscribed
        if (!subscriptions_ || (subscribed_oids.empty() && detail_level_ != catena::Device_DetailLevel_SUBSCRIPTIONS)) {
            return true;
        }
        
        // If we're in SUBSCRIPTIONS mode but have no subscriptions, nothing is subscribed
        if (detail_level_ == catena::Device_DetailLevel_SUBSCRIPTIONS && subscribed_oids.empty()) {
            return false;
        }
        
        // Check each subscription for exact or wildcard matches
        for (const auto& subscription_oid : subscribed_oids) {
            std::string oid = subscription_oid.substr(1);  // Remove leading slash
            if (param_name == oid) {
                return true;
            }
            
            // Check for wildcard match (ends with *)
            if (!oid.empty() && oid.back() == '*' && param_name.find(oid.substr(0, oid.size() - 1)) == 0) {
                return true;
            }
        }
        
        return false;
    };

    // Helper function to check if an item should be sent based on detail level and subscription
    auto shouldSendItem = [&](const std::string& oid) {
        // For non-IParam types, just check subscription status
        return detail_level_ == catena::Device_DetailLevel_FULL || 
               (detail_level_ == catena::Device_DetailLevel_SUBSCRIPTIONS && is_subscribed(oid));
    };

    // Only send non-minimal items in FULL mode or if explicitly subscribed in SUBSCRIPTION mode
    if (detail_level_ == catena::Device_DetailLevel_FULL || 
        (detail_level_ == catena::Device_DetailLevel_SUBSCRIPTIONS && !subscribed_oids.empty())) {
        
        // Send menus
        for (const auto& [group_name, menu_group] : menu_groups_) {
            for (const auto& [name, menu] : *menu_group->menus()) {
                std::string oid = group_name + "/" + name;
                if (shouldSendItem(oid)) {
                    co_yield component;
                    component.Clear();
                    ::catena::Menu* dstMenu = component.mutable_menu()->mutable_menu();
                    menu.toProto(*dstMenu);
                    component.mutable_menu()->set_oid(oid);
                }
            }
        }

        // Send language packs
        for (const auto& [language, language_pack] : language_packs_) {
            if (shouldSendItem(language)) {
                co_yield component;
                component.Clear();
                ::catena::LanguagePack* dstPack = component.mutable_language_pack()->mutable_language_pack();
                language_pack->toProto(*dstPack);
                component.mutable_language_pack()->set_language(language);
            }
        }

        // Send constraints
        for (const auto& [name, constraint] : constraints_) {
            if (shouldSendItem(name)) {
                co_yield component;
                component.Clear();
                ::catena::Constraint* dstConstraint = component.mutable_shared_constraint()->mutable_constraint();
                constraint->toProto(*dstConstraint);
                component.mutable_shared_constraint()->set_oid(name);
            }
        }
    }

    // Send parameters if authorized, and either in the minimal set or if subscribed to
    if (detail_level_ != catena::Device_DetailLevel_COMMANDS) {
        for (const auto& [name, param] : params_) {
            if (this->shouldSendParam(*param, is_subscribed(name), authz)) {
                co_yield component;
                component.Clear();
                ::catena::Param* dstParam = component.mutable_param()->mutable_param();
                param->toProto(*dstParam, authz);
                component.mutable_param()->set_oid(name);
            }
        }
    }

    // Send commands if authorized and in COMMANDS mode
    if (detail_level_ == catena::Device_DetailLevel_COMMANDS) {
        for (const auto& [name, param] : commands_) {
            if (this->shouldSendParam(*param, is_subscribed(name), authz)) {
                co_yield component;
                component.Clear();
                ::catena::Param* dstParam = component.mutable_command()->mutable_param();
                param->toProto(*dstParam, authz);
                component.mutable_command()->set_oid(name);
            }
        }
    }
    
    // return the last component
    co_return component;
}

bool Device::shouldSendParam(const IParam& param, bool is_subscribed, Authorizer& authz) const {
    // First check authorization
    if (!authz.readAuthz(param)) {
        return false;
    }

    // Then check detail level
    switch (detail_level_) {
        case Device_DetailLevel_NONE:
            return false;
        case Device_DetailLevel_MINIMAL:
            return param.getDescriptor().minimalSet();
        case Device_DetailLevel_FULL:
            return true;
        case Device_DetailLevel_SUBSCRIPTIONS:
            if (param.getDescriptor().minimalSet()) {
                return true;
            }
            return is_subscribed;
        case Device_DetailLevel_COMMANDS:
            return param.getDescriptor().isCommand();
        default:
            return false;
    }
}

