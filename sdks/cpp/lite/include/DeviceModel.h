#pragma once


#include <unordered_map>
#include <string>


namespace catena {
namespace lite {
class IParam;  // forward reference
class DeviceModel {
  public:
    DeviceModel() = default;
    virtual ~DeviceModel() = default;

    void AddParam(const std::string& name, IParam* param);

    IParam* GetParam(const std::string& name);

  private:
    std::unordered_map<std::string, IParam*> params_;
};
}  // namespace lite
}  // namespace catena
