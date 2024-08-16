#pragma once

#include <common/include/Path.h>
#include <common/include/Enums.h>
#include <common/include/vdk/signals.h>
#include <lite/include/Collection.h>

#include <lite/device.pb.h>

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <cassert>
#include <type_traits>

namespace catena::common {
  class IConstraint;    // forward reference
} 

namespace catena {
namespace lite {
  
class IParam;         // forward reference
class IMenuGroup;     // forward reference
class ILanguagePack;  // forward reference


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
     * @brief get a collection of items from the device
     * @param tag the tag of the collection to get
     * @return collection of items
     * @note tag can be CommmandTag, ParamTag, ConstraintTag, MenuGroupTag, or LanguagePackTag
     */
    template <typename TAG>
    Collection<typename TAG::type>& getCollection(TAG tag) {
      if constexpr(std::is_same_v<TAG, ParamTag>) {
        return params_;
      } else if constexpr(std::is_same_v<TAG, CommandTag>) {
        return commands_;
      } else if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        return constraints_;
      } else if constexpr(std::is_same_v<TAG, MenuGroupTag>) {
        return menu_groups_;
      } else if constexpr(std::is_same_v<TAG, LanguagePackTag>) {
        return language_packs_;
      }
    }

    /**
     * @brief retreive an item from the device by name
     * @param name path to the item relative to device
     * @return item can be IParameter, IConstraint, IMenuGroup, or ILanguagePack.
     * @note tag can be CommmandTag, ParamTag, ConstraintTag, MenuGroupTag, or LanguagePackTag
     */
    template <typename TAG>
    TAG::type* getItem(const std::string& name, TAG tag) const {
      if constexpr(std::is_same_v<TAG, ParamTag>) {
        return params_.getItem(name);
      } else if constexpr(std::is_same_v<TAG, CommandTag>) {
        return commands_.getItem(name);
      } else if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        return constraints_.getItem(name);
      } else if constexpr(std::is_same_v<TAG, MenuGroupTag>) {
        return menu_groups_.getItem(name);
      } else if constexpr(std::is_same_v<TAG, LanguagePackTag>) {
        return language_packs_.getItem(name);
      } else {
        return nullptr;
      }
    }

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

  public:
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByClient;
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByServer;

  private:
    uint32_t slot_;
    Device_DetailLevel detail_level_;
    Collection<catena::common::IConstraint> constraints_;
    Collection<IParam> params_;
    Collection<IMenuGroup> menu_groups_;
    Collection<IParam> commands_;
    Collection<ILanguagePack> language_packs_;
    std::vector<Scopes_e> access_scopes_;
    Scopes default_scope_;
    bool multi_set_enabled_;
    bool subscriptions_;

    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
