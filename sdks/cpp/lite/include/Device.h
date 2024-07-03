#pragma once

#include <common/include/Path.h>

#include <unordered_map>
#include <string>
#include <mutex>


namespace catena {
namespace lite {
class IParam;  // forward reference
class Device {
  public:
    class LockGuard {
      public:
        LockGuard(Device& dm) : dm_(dm) {
          dm_.mutex_.lock();
        }
        ~LockGuard() {
          dm_.mutex_.unlock();
        }
      private:
        Device& dm_;
    };
  friend class LockGuard;

  public:
    Device() = default;
    virtual ~Device() = default;

    void AddParam(const std::string& name, IParam* param);

    IParam* GetParam(catena::common::Path& path);

    IParam* GetParam(const std::string& name);

  private:
    std::unordered_map<std::string, IParam*> params_;
    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
