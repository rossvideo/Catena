#pragma once

/**
 * @file Device.h
 * @brief Device class definition
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-07-07
 */

#include <common/include/Path.h>
#include <common/include/Enums.h>
#include <common/include/vdk/signals.h>

#include <lite/include/IParam.h>
#include <lite/include/IConstraint.h>
#include <lite/include/Tags.h>  

#include <lite/device.pb.h>

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
    void toProto(::catena::Device& dst, bool shallow = true) const;

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    template <typename TAG>
    void addItem(const std::string& key, typename TAG::type* item) {
        if constexpr (std::is_same_v<TAG, ParamTag>) {
            params_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, ConstraintTag>) {
            constraints_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, MenuGroupTag>) {
            menu_groups_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, CommandTag>) {
            commands_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, LanguagePackTag>) {
            language_packs_[key] = item;
        }
    }

    /**
     * @brief gets an item from one of the collections owned by the device
     * @return nullptr if the item is not found, otherwise the item
     */
    template <typename TAG>
    typename TAG::type* getItem(const std::string& key) const {
        GET_ITEM(ParamTag, params_)
        GET_ITEM(ConstraintTag, constraints_)
        GET_ITEM(MenuGroupTag, menu_groups_)
        GET_ITEM(CommandTag, commands_)
        GET_ITEM(LanguagePackTag, language_packs_)
        return nullptr;
    }

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

  private:
    uint32_t slot_;
    Device_DetailLevel detail_level_;
    std::unordered_map<std::string, IConstraint*> constraints_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, IMenuGroup*> menu_groups_;
    std::unordered_map<std::string, IMenuGroup*> commands_;
    std::unordered_map<std::string, ILanguagePack*> language_packs_;
    std::vector<Scopes_e> access_scopes_;
    Scopes default_scope_;
    bool multi_set_enabled_;
    bool subscriptions_;

    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
