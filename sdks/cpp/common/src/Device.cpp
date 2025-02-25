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

catena::exception_with_status Device::multiSetValue (catena::MultiSetValuePayload src, Authorizer& authz) {
    // Transaction status.
    catena::exception_with_status ans{"", catena::StatusCode::OK};
    // Vector of verified set value requests.
    std::vector<SetValueRequest> requests;
    // Vector of paths to verified appends.
    std::vector<Path*> appends;
    // Failing multiSetValue if multi set is disabled.
    if (multi_set_enabled_ == false && src.values().size() > 1) {
        ans = catena::exception_with_status("Multi-set is disabled ", catena::StatusCode::PERMISSION_DENIED);
    } else {
        // Looping through and validating set value requests.
        for (auto& setValuePayload : src.values()) {
            try {
                requests.push_back(SetValueRequest{std::make_unique<Path>(setValuePayload.oid()), &setValuePayload});
                Path* path = requests.back().first.get();
                std::unique_ptr<IParam> param;
                /*
                 * Path ends in "/-". We need to add to the back of the parent
                 * param's internal vector and get its index. Index is required
                 * as too many appends will cause the vector to be reallocated,
                 * making all pointers to that param's values invalid.
                 */
                if (path->back_is_index() ? path->back_as_index() == catena::common::Path::kEnd : false) {
                    path->popBack();
                    param = getParam(*path, ans, authz);
                    if (param != nullptr) {
                        uint32_t oidIndex = param->size();
                        param = param->addBack(authz, ans);
                        if (param != nullptr) {
                            path->push_back(oidIndex);
                            appends.push_back(path);
                        }
                    }
                // Path does not end in "/-", get param normally.
                } else {
                    param = getParam(*path, ans, authz);
                }
                // Rewind path to start for future use.
                path->rewind();
                // Validate that the request is valid.
                if (param == nullptr) {
                    break;
                }
                else if (!authz.readAuthz(*param)) {
                    ans = catena::exception_with_status("Not authorized to read the param " + setValuePayload.oid(), catena::StatusCode::PERMISSION_DENIED);
                    break;
                }
                else if (!authz.writeAuthz(*param)) {
                    ans = catena::exception_with_status("Not authorized to write to param " + setValuePayload.oid(), catena::StatusCode::PERMISSION_DENIED);
                    break;
                }
                else if (!param->validateSize(setValuePayload.value())) {
                    ans = catena::exception_with_status("Value exceeds maximum length of " + setValuePayload.oid(), catena::StatusCode::OUT_OF_RANGE);
                    break;
                }
            // Try-catch to catch exceptions thrown by the Path constructor.
            } catch (const catena::exception_with_status& why) {
                ans = catena::exception_with_status(why.what(), why.status);
                break;
            }
        }
        // Something went wrong above. Rolling back all appends before returning.
        if (ans.status != catena::StatusCode::OK) { 
            for (Path* append_path : appends) {
                append_path->popBack();
                getParam(*append_path, ans, authz)->popBack(authz);
            }
        // Everything is valid, commiting all set value requests.
        } else if (ans.status == catena::StatusCode::OK) {
            for (auto& [path, setValuePayload] : requests) {
                std::unique_ptr<IParam> param = getParam(*path, ans, authz);
                ans = param->fromProto(setValuePayload->value(), authz);
                valueSetByClient.emit(setValuePayload->oid(), param.get(), 0);
            }   
        }
    }
    return ans;
}

catena::exception_with_status Device::setValue (const std::string& jptr, catena::Value& src, Authorizer& authz) {
    catena::MultiSetValuePayload setValues;
    catena::SetValuePayload* setValuePayload = setValues.add_values();
    setValuePayload->set_oid(jptr);
    setValuePayload->set_allocated_value(&src);
    return multiSetValue(setValues, authz);
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

    // if we're not doing a shallow copy, we need to copy all the Items
    google::protobuf::Map<std::string, ::catena::Param> dstParams{};
    for (const auto& [name, param] : params_) {
        bool should_send = authz.readAuthz(*param);
        if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            should_send = should_send && param->getDescriptor().minimalSet();
        }
        if (should_send) {
            ::catena::Param dstParam{};
            param->toProto(dstParam, authz);
            dstParams[name] = dstParam;
        }
    }
    dst.mutable_params()->swap(dstParams); // n.b. lowercase swap, not an error

    // make deep copy of the commands
    google::protobuf::Map<std::string, ::catena::Param> dstCommands{};
    for (const auto& [name, command] : commands_) {
        bool should_send = authz.readAuthz(*command);
        if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            should_send = should_send && command->getDescriptor().minimalSet();
        }
        if (should_send) {
            ::catena::Param dstCommand{};
            command->toProto(dstCommand, authz); // uses param's toProto method
            dstCommands[name] = dstCommand;
        }
    }
    dst.mutable_commands()->swap(dstCommands); // n.b. lowercase swap, not an error

    // make deep copy of the constraints
    google::protobuf::Map<std::string, ::catena::Constraint> dstConstraints{};
    if (detail_level_ != catena::Device_DetailLevel_MINIMAL) {
        for (const auto& [name, constraint] : constraints_) {
            ::catena::Constraint dstConstraint{};
            constraint->toProto(dstConstraint);
            dstConstraints[name] = dstConstraint;
        }
    }
    dst.mutable_constraints()->swap(dstConstraints); // n.b. lowercase swap, not an error

    // make deep copy of the language packs
    ::catena::LanguagePacks dstPacks{};
    if (detail_level_ != catena::Device_DetailLevel_MINIMAL) {
        for (const auto& [name, pack] : language_packs_) {
            ::catena::LanguagePack dstPack{};
            pack->toProto(dstPack);
            dstPacks.mutable_packs()->insert({name, dstPack});
        }
    }
    dst.mutable_language_packs()->Swap(&dstPacks); // N.B uppercase Swap, not an error

    // make a copy of the menu groups
    google::protobuf::Map<std::string, ::catena::MenuGroup> dstMenuGroups{};
    if (detail_level_ != catena::Device_DetailLevel_MINIMAL) {
        for (const auto& [name, group] : menu_groups_) {
            ::catena::MenuGroup dstGroup{};
            group->toProto(dstGroup, false);
            dstMenuGroups[name] = dstGroup;
        }
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

Device::DeviceSerializer Device::getComponentSerializer(Authorizer& authz, const std::vector<std::string> subscribed_oids, bool shallow) const {
    std::cout << "getComponentSerializer received vector size: " << subscribed_oids.size() << std::endl;
    for (const auto& oid : subscribed_oids) {
        std::cout << "Received OID: '" << oid << "'" << std::endl;
    }
    catena::DeviceComponent component{};
    
    // Always send basic device info first
    catena::Device* dst = component.mutable_device();
    dst->set_slot(slot_);
    dst->set_detail_level(detail_level_);
    *dst->mutable_default_scope() = default_scope_;
    dst->set_multi_set_enabled(multi_set_enabled_);
    dst->set_subscriptions(subscriptions_);

    for (auto& scope : access_scopes_) {
        dst->add_access_scopes(scope);
    }

    if (shallow) {
        // In shallow mode with MINIMAL detail level, only include minimal_set parameters
        if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            // Add minimal set parameters
            google::protobuf::Map<std::string, ::catena::Param> dstParams{};
            for (const auto& [name, param] : params_) {
                if (authz.readAuthz(*param) && param->getDescriptor().minimalSet()) {
                    ::catena::Param dstParam{};
                    param->toProto(dstParam, authz);
                    dstParams[name] = dstParam;
                }
            }
            dst->mutable_params()->swap(dstParams);
        } else {
            // For FULL mode, include everything
            toProto(*dst, authz, shallow);
        }
        co_return component;
    }

    // Helper function to check if an OID is subscribed
    auto is_subscribed = [&subscribed_oids, this](const std::string& oid) {
        std::cout << "Checking subscription for OID: '" << oid << "'" << std::endl;
        std::cout << "Current detail level: " << detail_level_ << std::endl;

        // If subscriptions are disabled or in FULL mode, allow all
        if (!subscriptions_ || detail_level_ == catena::Device_DetailLevel_FULL) {
            std::cout << "Subscriptions disabled or FULL mode, allowing all" << std::endl;
            return true;
        }

        // If no subscriptions specified and in MINIMAL mode, return true to let minimal_set be the deciding factor
        if (subscribed_oids.empty() && detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            std::cout << "No subscriptions specified and in MINIMAL mode, letting minimal_set be the deciding factor" << std::endl;
            return true;
        }

        // If we have explicit subscriptions, check them
        if (!subscribed_oids.empty()) {
            std::cout << "Checking against " << subscribed_oids.size() << " subscribed OIDs:" << std::endl;
            for (const auto& subscribed : subscribed_oids) {
                std::cout << "    - '" << subscribed << "'" << std::endl;
            }
            
            auto it = std::find_if(subscribed_oids.begin(), subscribed_oids.end(),
                [&oid](const std::string& subscribed_oid) {
                    bool matches = !subscribed_oid.empty() && oid.find(subscribed_oid) != std::string::npos;
                    std::cout << "Comparing with '" << subscribed_oid << "': " << (matches ? "match" : "no match") << std::endl;
                    return matches;
                });
            bool result = it != subscribed_oids.end();
            std::cout << "Final result: " << (result ? "subscribed" : "not subscribed") << std::endl;
            return result;
        }

        return false;
    };

    // Send menu groups if subscribed or if subscriptions are disabled
    for (auto& [menu_group_name, menu_group] : menu_groups_) {
        if (is_subscribed(menu_group_name)) {
            ::catena::MenuGroup* dstMenuGroup = &(*dst->mutable_menu_groups())[menu_group_name];
            menu_group->toProto(*dstMenuGroup, true);
        }
    }

    // Send language packs if subscribed or if subscriptions are disabled
    for (const auto& [language, language_pack] : language_packs_) {
        if (is_subscribed(language)) {
            co_yield component;
            component.Clear();
            ::catena::LanguagePack* dstPack = component.mutable_language_pack()->mutable_language_pack();
            language_pack->toProto(*dstPack);
            component.mutable_language_pack()->set_language(language);
        }
    }

    // Send constraints if subscribed or if subscriptions are disabled
    for (const auto& [name, constraint] : constraints_) {
        if (is_subscribed(name)) {
            co_yield component;
            component.Clear();
            ::catena::Constraint* dstConstraint = component.mutable_shared_constraint()->mutable_constraint();
            constraint->toProto(*dstConstraint);
            component.mutable_shared_constraint()->set_oid(name);
        }
    }

    // Send params if authorized and either subscribed or if subscriptions are disabled
    for (const auto& [name, param] : params_) {
        bool should_send = authz.readAuthz(*param) && is_subscribed(name);
        if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            std::cout << "Checking minimal set for param: '" << name << "'" << std::endl;
            should_send = should_send && param->getDescriptor().minimalSet();
            std::cout << "Minimal set check result: " << (should_send ? "true" : "false") << std::endl;
        }
        if (should_send) {
            co_yield component;
            component.Clear();
            ::catena::Param* dstParam = component.mutable_param()->mutable_param();
            param->toProto(*dstParam, authz);
            component.mutable_param()->set_oid(name);
        }
    }

    // Send commands if authorized and either subscribed or if subscriptions are disabled
    for (const auto& [name, command] : commands_) {
        bool should_send = authz.readAuthz(*command) && is_subscribed(name);
        if (detail_level_ == catena::Device_DetailLevel_MINIMAL) {
            should_send = should_send && command->getDescriptor().minimalSet();
        }
        if (should_send) {
            co_yield component;
            component.Clear();
            ::catena::Param* dstCommand = component.mutable_command()->mutable_param();
            command->toProto(*dstCommand, authz);
            component.mutable_command()->set_oid(name);
        }
    }

    // Send menus if subscribed or if subscriptions are disabled
    for (const auto& [name, menu_groups_] : menu_groups_) {
        for (const auto& [menu_name, menu] : *menu_groups_->menus()) {
            std::string oid = name + "/" + menu_name;
            if (is_subscribed(oid)) {
                co_yield component;
                component.Clear();
                ::catena::Menu* dstMenu = component.mutable_menu()->mutable_menu();
                menu.toProto(*dstMenu);
                component.mutable_menu()->set_oid(oid);
            }
        }
    }
    
    // return the last component
    co_return component;
}

