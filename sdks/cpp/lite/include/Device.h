#pragma once

#include <common/include/Path.h>
#include <common/include/Enums.h>

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <cassert>
#include <type_traits>


namespace catena {
namespace lite {
class IParam;         // forward reference
class IConstraint;    // forward reference
class IMenuGroup;     // forward reference
class ILanguagePack;  // forward reference
class Device {
  public:
    class LockGuard {
      public:
        LockGuard(Device& dm) : dm_(dm) { dm_.mutex_.lock(); }
        ~LockGuard() { dm_.mutex_.unlock(); }

      private:
        Device& dm_;
    };
    friend class LockGuard;

    using Scopes_e = catena::common::Scopes_e;
    using DetailLevel_e = catena::common::DetailLevel_e;
    struct ParamTag {
        using type = IParam;
    };
    struct CommandTag {
        using type = IParam;
    };
    struct ConstraintTag {
        using type = IConstraint;
    };
    struct MenuGroupTag {
        using type = IMenuGroup;
    };
    struct LanguagePackTag {
        using type = ILanguagePack;
    };

  public:
    /**
     * @brief Construct a new Device object
     */
    Device() = default;

    /**
     * @brief Construct a new Device object
     */
    Device(uint32_t slot, DetailLevel_e detail_level, std::vector<Scopes_e> access_scopes,
           Scopes_e default_scope, bool multiset_enabled, bool subscriptions)
        : slot_(slot), detail_level_(detail_level), access_scopes_(access_scopes),
          default_scope_(default_scope), multiset_enabled_(multiset_enabled), subscriptions_(subscriptions) {}

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
     * @brief add an item to the device.
     * item can be a parameter, constraint, menu group, command, or language pack.
     * @param name the name of the item
     * @param item the item to add
     */
    template <typename TAG> void addItem(const std::string& name, TAG::type* item, TAG tag) {
      assert(item != nullptr);
      if constexpr(std::is_same_v<TAG, ParamTag>) {
        params_[name] = item;
      } else if constexpr(std::is_same_v<TAG, CommandTag>) {
        commands_[name] = item;
      } else if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        constraints_[name] = item;
      } else if constexpr(std::is_same_v<TAG, MenuGroupTag>) {
        menu_groups_[name] = item;
      } else if constexpr(std::is_same_v<TAG, LanguagePackTag>) {
        language_packs_[name] = item;
      } else {
        // static_assert(false, "Unknown TAG type");
      }
    }

    /**
     * @brief retreive an item from the device by json pointer.
     * item can be an IParameter, IConstraint, IMenuGroup, or ILanguagePack.
     * @param path path to the item relative to device.<items>
     */
    template <typename TAG> TAG::type* getItem(const std::string& name, TAG tag) const {
      if constexpr(std::is_same_v<TAG, ParamTag>) {
        auto it = params_.find(name);
        if (it != params_.end()) {
          return it->second;
        }
      } else if constexpr(std::is_same_v<TAG, CommandTag>) {
        auto it = commands_.find(name);
        if (it != commands_.end()) {
          return it->second;
        }
      } else if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        auto it = constraints_.find(name);
        if (it != constraints_.end()) {
          return it->second;
        }
      } else if constexpr(std::is_same_v<TAG, MenuGroupTag>) {
        auto it = menu_groups_.find(name);
        if (it != menu_groups_.end()) {
          return it->second;
        }
      } else if constexpr(std::is_same_v<TAG, LanguagePackTag>) {
        auto it = language_packs_.find(name);
        if (it != language_packs_.end()) {
          return it->second;
        }
      } else {
        // static_assert(false, "Unknown TAG type");
      }
      return nullptr;
    }


  private:
    uint32_t slot_;
    DetailLevel_e detail_level_;
    std::unordered_map<std::string, IConstraint*> constraints_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, IMenuGroup*> menu_groups_;
    std::unordered_map<std::string, IParam*> commands_;
    std::unordered_map<std::string, ILanguagePack*> language_packs_;
    std::vector<Scopes_e> access_scopes_;
    Scopes_e default_scope_;
    bool multiset_enabled_;
    bool subscriptions_;

    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
