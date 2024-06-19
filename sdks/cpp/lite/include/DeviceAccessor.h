
#pragma once

#include "Param.h"
#include <example.tiny.json.h>


class DeviceAccessor {

  public:
    DeviceAccessor();

    std::unique_ptr<catena::Value> getSerializedValue(std::string oid) const;

  private:
    tiny::DeviceModel deviceModel_;
};