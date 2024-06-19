
#include "DeviceAccessor.h"

DeviceAccessor::DeviceAccessor(){};

std::unique_ptr<catena::Value> DeviceAccessor::getSerializedValue(std::string oid) const {
    return deviceModel_.paramInfoMap.at(oid).param->getSerializedValue();
}