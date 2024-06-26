#pragma once

#include <common/include/Path.h>

#include <unordered_map>
#include <string>
#include <mutex>


namespace catena {
namespace lite {
class IParam;  // forward reference
class DeviceModel {
  public:
    class LockGuard {
      public:
        LockGuard(DeviceModel& dm) : dm_(dm) {
          dm_.mutex_.lock();
        }
        ~LockGuard() {
          dm_.mutex_.unlock();
        }
      private:
        DeviceModel& dm_;
    };
  friend class LockGuard;

  public:
    DeviceModel() = default;
    virtual ~DeviceModel() = default;

    void AddParam(const std::string& name, IParam* param);

    IParam* GetParam(catena::common::Path& path);

  private:
    std::unordered_map<std::string, IParam*> params_;
    mutable std::mutex mutex_;
};
}  // namespace lite
}  // namespace catena
