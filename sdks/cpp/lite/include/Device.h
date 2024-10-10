#pragma once

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

/**
 * @file Device.h
 * @brief Device class definition
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <Path.h>
#include <Enums.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <ILanguagePack.h>
#include <Tags.h>  
#include <Status.h>

// protobuf interface
#include <interface/device.pb.h>

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <cassert>
#include <type_traits>

namespace catena {
namespace lite {

/**
 * @brief Device class
 * Implements the Device interface defined in the protobuf.
 */
class Device {
  public:
    /**
     * @brief LockGuard is a helper class to lock and unlock the device mutex
     */
    class LockGuard {
      public:
        LockGuard(Device& dm) : dm_(dm) { dm_.mutex_.lock(); }
        ~LockGuard() { dm_.mutex_.unlock(); }

      private:
        Device& dm_;
    };
    friend class LockGuard;

    /**
     * @brief convenience type aliases to types of objects contained in the device
     */
    using Scopes_e = catena::common::Scopes_e;
    using Scopes = catena::common::Scopes;
    using DetailLevel_e = catena::Device_DetailLevel;
    using DetailLevel = catena::common::DetailLevel;
    using IParam = catena::common::IParam;

  public:
    /**
     * @brief Construct a new Device object
     */
    Device() = default;

    /**
     * @brief Construct a new Device object
     */
    Device(uint32_t slot, Device_DetailLevel detail_level, std::vector<Scopes_e> access_scopes,
      Scopes_e default_scope, bool multi_set_enabled, bool subscriptions)
      : slot_{slot}, detail_level_{detail_level}, access_scopes_{access_scopes},
      default_scope_{default_scope}, multi_set_enabled_{multi_set_enabled}, subscriptions_{subscriptions} {}

    /**
     * @brief Destroy the Device object
     */
    virtual ~Device() = default;

    /**
     * @brief set the slot number of the device
     * @param slot the slot number of the device
     */
    inline void slot(const uint32_t slot) { slot_ = slot; }

    /**
     * @brief get the slot number of the device
     * @return slot number
     */
    inline uint32_t slot() const { return slot_; }

    /**
     * @brief set the detail level of the device
     * @param detail_level the detail level of the device
     */
    inline void detail_level(const DetailLevel_e detail_level) { detail_level_ = detail_level; }

    /**
     * @brief get the detail level of the device
     * @return DetailLevel_e
     */
    inline DetailLevel_e detail_level() const { return detail_level_; }

    inline std::string getDefaultScope() const { return default_scope_.toString(); }

    /**
     * @brief Create a protobuf representation of the device.
     * @param dst the protobuf representation of the device.
     * @param shallow if true, only the top-level info is copied, params, commands etc 
     * are not copied. Design intent is to permit large models to stream their parameters
     * instead of sending a huge device model in one big lump.
     * 
     * N.B. This method is not thread-safe. It is the caller's responsibility to ensure
     * that the device is not modified while this method is running. This class provides
     * a LockGuard helper class to make this easier.
     */
    void toProto(::catena::Device& dst, std::vector<std::string>& clientScopes, bool shallow = true) const;

    /**
     * @brief Create a protobuf representation of the language packs.
     * @param packs the protobuf representation of the language packs.
     */
    void toProto(::catena::LanguagePacks& packs) const;

    /**
     * @brief Create a protobuf representation of the language list.
     * @param list the protobuf representation of the language list.
     */
    void toProto(::catena::LanguageList& list) const;

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    template <typename TAG>
    void addItem(const std::string& key, typename TAG::type* item) {
        if constexpr (std::is_same_v<TAG, common::ParamTag>) {
            params_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::ConstraintTag>) {
            constraints_[key] = item;
        }
        // if constexpr (std::is_same_v<TAG, common::MenuGroupTag>) {
        //     menu_groups_[key] = item;
        // }
        if constexpr (std::is_same_v<TAG, common::CommandTag>) {
            commands_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::LanguagePackTag>) {
            language_packs_[key] = item;
        }
    }

    /**
     * @brief gets an item from one of the collections owned by the device
     * @return nullptr if the item is not found, otherwise the item
     */
    template <typename TAG>
    typename TAG::type* getItem(const std::string& key) const {

        GET_ITEM(common::ParamTag, params_)
        GET_ITEM(common::ConstraintTag, constraints_)
        // GET_ITEM(common::MenuGroupTag, menu_groups_)
        GET_ITEM(common::CommandTag, commands_)
        GET_ITEM(common::LanguagePackTag, language_packs_)
        return nullptr;
    }

    /**
     * @brief get a parameter by oid
     * @param fqoid the fully qualified oid of the parameter
     * @return a unique pointer to the parameter, or nullptr if it does not exist
     */
    std::unique_ptr<IParam> getParam(const std::string& fqoid, catena::exception_with_status& status) const;

    /**
     * @brief get a command by oid
     * @param fqoid the fully qualified oid of the command
     * @return a unique pointer to the command, or nullptr if it does not exist
     */
    std::unique_ptr<IParam> getCommand(const std::string& fqoid, catena::exception_with_status& status) const;

    /**
     * @brief deserialize a protobuf value object into the parameter value
     * pointed to by jptr.
     * @param jptr, json pointer to the part of the device model to update.
     * @param src, the value to update the parameter with.
     * @todo consider using move semantics on the value parameter to emphasize new ownership.
     * @return an exception_with_status with status set OK if successful, otherwise an error.
     * Intention is to for SetValue RPCs / API calls to be serviced by this method.
     * @note on success, this method will emit the valueSetByClient signal.
     */
    catena::exception_with_status setValue (const std::string& jptr, catena::Value& src);

    /**
     * @brief serialize the parameter value to protobuf
     * @param jptr, json pointer to the part of the device model to serialize.
     * @param dst, the protobuf value to serialize to.
     * @return an exception_with_status with status set OK if successful, otherwise an error.
     * Intention is to for GetValue RPCs / API calls to be serviced by this method.
     */
    catena::exception_with_status getValue (const std::string& jptr, catena::Value& value);

  public:
    /**
     * @brief signal emitted when a value is set by the client.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByClient;
    
    /**
     * @brief signal emitted when a value is set by the server, or business logic.
     * Intended recipient is the connection manager.
     */
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByServer;

    static const std::vector<std::string> kAuthzDisabled;
  private:
    uint32_t slot_;
    Device_DetailLevel detail_level_;
    std::unordered_map<std::string, catena::common::IConstraint*> constraints_;
    std::unordered_map<std::string, IParam*> params_;
    // std::unordered_map<std::string, IMenuGroup*> menu_groups_;
    std::unordered_map<std::string, IParam*> commands_;
    std::unordered_map<std::string, common::ILanguagePack*> language_packs_;
    std::vector<Scopes_e> access_scopes_;
    Scopes default_scope_;
    bool multi_set_enabled_;
    bool subscriptions_;
    
    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
