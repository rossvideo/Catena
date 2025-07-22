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
#include <IParam.h>
#include <Device.h>
#include <LanguagePack.h>
#include <ParamWithValue.h>
#include <IParamDescriptor.h>
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
        ans = catena::exception_with_status("Multi-set is disabled for the device in slot " + std::to_string(slot_), catena::StatusCode::PERMISSION_DENIED);
    } else {
        std::vector<std::string> affectedOids;

        // Looping through and validating set value requests.
        for (const catena::SetValuePayload& setValuePayload : src.values()) {
            try {
                // First validate no overlapping actions.
                for (const auto& oid : affectedOids) {
                    if (!(oid.ends_with("/-") && (setValuePayload.oid().ends_with("/-"))) &&
                       (oid.starts_with(setValuePayload.oid()) || setValuePayload.oid().starts_with(oid))) {
                        ans = catena::exception_with_status("Overlapping actions for " + oid + " and " + setValuePayload.oid(), catena::StatusCode::INVALID_ARGUMENT);
                        break;
                    }
                }
                // Then ensure the request will go through if fromProto is called.
                if (ans.status == catena::StatusCode::OK) {
                    affectedOids.push_back(setValuePayload.oid());
                    // Getting param, or parent param if the final segment is an index.
                    Path path(setValuePayload.oid());
                    Path::Index index = Path::kNone;
                    if (path.back_is_index()) {
                        index = path.back_as_index();
                        path.popBack();
                    }
                    std::unique_ptr<IParam> param = getParam(path, ans, authz);
                    // Validating setValue operation.
                    if (!param || !param->validateSetValue(setValuePayload.value(), index, authz, ans)) {
                        break;
                    }
                }
            } catch (const catena::exception_with_status& why) {
                ans = catena::exception_with_status(why.what(), why.status);
                break;
            }
        }
        // Resetting trackers regardless of whether something went wrong or not.
        for (const std::string& oid : affectedOids) {
            // Creating another exception_with_status as to not overwrite ans.
            catena::exception_with_status status{"", catena::StatusCode::OK};
            // Resetting param's trackers, or parent param's if back is index.
            Path path(oid);
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
            valueSetByClient.emit(setValuePayload.oid(), param.get());
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
        if (path.back_is_index() && path.back_as_index() == catena::common::Path::kEnd) {
            // Index is "-"
            ans = catena::exception_with_status("Index out - of bounds in path " + jptr, catena::StatusCode::OUT_OF_RANGE);
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
    } catch (const std::exception& e) {
        ans = catena::exception_with_status(e.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        ans = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    return ans;
}

catena::exception_with_status Device::getLanguagePack(const std::string& languageId, ComponentLanguagePack& pack) const {
    catena::exception_with_status ans{"", catena::StatusCode::OK};

    try {
        if (languageId.empty()) {
            ans = catena::exception_with_status("Language ID is empty", catena::StatusCode::INVALID_ARGUMENT);
        } else if (!language_packs_.contains(languageId)) {
            ans = catena::exception_with_status("Language pack '" + languageId + "' not found", catena::StatusCode::NOT_FOUND);
        } else {
            pack.set_language(languageId);
            auto languagePack = pack.mutable_language_pack();
            language_packs_.at(languageId)->toProto(*languagePack);
            ans = catena::exception_with_status("", catena::StatusCode::OK);
        }
    } catch (const std::exception& e) {
        ans = catena::exception_with_status(e.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        ans = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    return ans;
}

catena::exception_with_status Device::addLanguage (catena::AddLanguagePayload& language, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    auto& name = language.language_pack().name();
    auto& id = language.id();
    // Admin scope required.
    if (!authz.hasAuthz(Scopes().getForwardMap().at(Scopes_e::kAdmin) + ":w")) {
        ans = catena::exception_with_status("Not authorized to add language", catena::StatusCode::PERMISSION_DENIED);
    // Making sure LanguagePack is properly formatted.
    } else if (name.empty() || id.empty()) {
        ans = catena::exception_with_status("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
    // Cannot overwrite non-field upgrade language packs.
    } else if (language_packs_.contains(id) && !added_packs_.contains(id)) {
        ans = catena::exception_with_status("Cannot overwrite language pack shipped with device", catena::StatusCode::PERMISSION_DENIED);
    // Adding the language pack.
    } else {
        // added_packs_ here to maintain ownership in device scope.
        added_packs_[id] = std::make_shared<LanguagePack>(id, name, LanguagePack::ListInitializer{}, *this);
        language_packs_[id]->fromProto(language.language_pack());      
        // Pushing update to connect gRPC.
        languageAddedPushUpdate.emit(language_packs_[id]);
    }
    return ans;
}

catena::exception_with_status Device::removeLanguage(const std::string& languageId, Authorizer& authz) {
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    // Admin scope required.
    if (!authz.hasAuthz(Scopes().getForwardMap().at(Scopes_e::kAdmin) + ":w")) {
        ans = catena::exception_with_status("Not authorized to delete language", catena::StatusCode::PERMISSION_DENIED);
    // Cannot change shipped language packs.
    } else if (language_packs_.contains(languageId) && !added_packs_.contains(languageId)) {
        ans = catena::exception_with_status("Cannot delete language pack shipped with device", catena::StatusCode::PERMISSION_DENIED);
    // Nothing to delete
    } else if (!language_packs_.contains(languageId)) {
        ans = catena::exception_with_status("Language pack '" + languageId + "' not found", catena::StatusCode::NOT_FOUND);
    // Removing the language pack.
    } else {
        added_packs_.erase(languageId);
        language_packs_.erase(languageId);
        // Push update???
    }
    return ans;
}

std::unique_ptr<IParam> Device::getParam(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) const {
    // The Path constructor will throw an exception if the json pointer is invalid, so we use a try catch block to catch it.
    catena::common::Path path(fqoid);
    return getParam(path, status, authz);
}

std::unique_ptr<IParam> Device::getParam(catena::common::Path& path, catena::exception_with_status& status, Authorizer& authz) const {
    if (path.empty()) {
        status = catena::exception_with_status("Invalid json pointer " + path.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
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
                //By default, names don't have a leading slash, so we add it here.
                std::string path_name = "/" + name;
                Path path(path_name);
                auto param_ptr = getParam(path, status, authz);  
                if (param_ptr) {
                    result.push_back(std::move(param_ptr));
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
                status = catena::exception_with_status("Command not found: " + fqoid, catena::StatusCode::NOT_FOUND);
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

std::unique_ptr<Device::IDeviceSerializer> Device::getComponentSerializer(Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow) const {
    return std::make_unique<Device::DeviceSerializer>(getDeviceSerializer(authz, subscribedOids, dl, shallow));
}

Device::DeviceSerializer Device::getDeviceSerializer(Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow) const {
    // Sanitizing if trying to use SUBSCRIPTIONS mode with subscriptions disabled
    if (dl == catena::Device_DetailLevel_SUBSCRIPTIONS && !subscriptions_) {
        throw catena::exception_with_status("Subscriptions are not enabled for this device", catena::StatusCode::INVALID_ARGUMENT);
    }
    
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

    if (dl != catena::Device_DetailLevel_NONE) {
        // // Helper function to check if an OID is subscribed
        auto isSubscribed = [&subscribedOids, dl, this](const std::string& paramName) {
            if (dl != catena::Device_DetailLevel_SUBSCRIPTIONS) {
                return true;
            } else {
                // Check each subscription for exact or wildcard matches
                for (const auto& subscribedOid : subscribedOids) {
                    // Remove leading slash
                    std::string oid = subscribedOid.substr(1);
                    // Check for exact match.
                    if (paramName == oid) {
                        return true;
                    }
                    // Check for wildcard match (ends with *)
                    if (!oid.empty() && oid.back() == '*' && paramName.find(oid.substr(0, oid.size() - 1)) == 0) {
                        return true;
                    }
                }
            }
            return false;
        };

        // // Only send non-minimal items in FULL mode or if explicitly subscribed in SUBSCRIPTION mode
        if (dl == catena::Device_DetailLevel_FULL || 
            dl == catena::Device_DetailLevel_SUBSCRIPTIONS) {
            
            // Send menus
            for (const auto& [groupGame, menuGroup] : menu_groups_) {
                for (const auto& [name, menu] : *menuGroup->menus()) {
                    std::string oid = groupGame + "/" + name;
                    if (isSubscribed(oid)) {
                        co_yield component;
                        component.Clear();
                        ::catena::Menu* dstMenu = component.mutable_menu()->mutable_menu();
                        menu->toProto(*dstMenu);
                        component.mutable_menu()->set_oid(oid);
                    }
                }
            }

            // Send language packs
            for (const auto& [language, languagePack] : language_packs_) {
                if (isSubscribed(language)) {
                    co_yield component;
                    component.Clear();
                    ::catena::LanguagePack* dstPack = component.mutable_language_pack()->mutable_language_pack();
                    languagePack->toProto(*dstPack);
                    component.mutable_language_pack()->set_language(language);
                }
            }

            // Send constraints
            for (const auto& [name, constraint] : constraints_) {
                if (isSubscribed(name)) {
                    co_yield component;
                    component.Clear();
                    ::catena::Constraint* dstConstraint = component.mutable_shared_constraint()->mutable_constraint();
                    constraint->toProto(*dstConstraint);
                    component.mutable_shared_constraint()->set_oid(name);
                }
            }
        }

        // Send parameters if authorized, and either in the minimal set or if subscribed to
        if (dl != catena::Device_DetailLevel_COMMANDS) {
            for (const auto& [name, param] : params_) {
                if (authz.readAuthz(*param) &&
                    ((dl == catena::Device_DetailLevel_FULL) ||
                     (param->getDescriptor().minimalSet()) ||
                     (dl == catena::Device_DetailLevel_SUBSCRIPTIONS && isSubscribed(name)))) {
                    co_yield component;
                    component.Clear();
                    ::catena::Param* dstParam = component.mutable_param()->mutable_param();
                    param->toProto(*dstParam, authz);
                    component.mutable_param()->set_oid(name);
                }
            }
        // Send commands if authorized and in COMMANDS mode
        } else {
            for (const auto& [name, param] : commands_) {
                if (authz.readAuthz(*param)) {
                    co_yield component;
                    component.Clear();
                    ::catena::Param* dstParam = component.mutable_command()->mutable_param();
                    param->toProto(*dstParam, authz);
                    component.mutable_command()->set_oid(name);
                }
            }
        }
    }
    // return the last component
    co_return component;
}

bool Device::shouldSendParam(const IParam& param, bool is_subscribed, Authorizer& authz) const {
    bool should_send = false;

    //Detail level casted to ints to avoid warnings about comparing enums on deprecated ASIO versions
    int casted_detail_level = static_cast<int>(detail_level_);

    // First check authorization
    if (authz.readAuthz(param)) {
        if (casted_detail_level != static_cast<int>(catena::Device_DetailLevel_NONE)) {
            should_send = 
                (casted_detail_level == static_cast<int>(catena::Device_DetailLevel_MINIMAL) && param.getDescriptor().minimalSet()) ||
                (casted_detail_level == static_cast<int>(catena::Device_DetailLevel_FULL)) ||
                (casted_detail_level == static_cast<int>(catena::Device_DetailLevel_SUBSCRIPTIONS) && (param.getDescriptor().minimalSet() || is_subscribed)) ||
                (casted_detail_level == static_cast<int>(catena::Device_DetailLevel_COMMANDS) && param.getDescriptor().isCommand());
        }
    }

    return should_send;
}

