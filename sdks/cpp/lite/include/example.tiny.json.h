#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Meta.h>
#include <TypeTraits.h>
#include <string>
#include <vector>
#include <Param.h>
#include <map>

namespace tiny {
class DeviceModel {
  public:
  DeviceModel();
  std::map<std::string, ParamInfo> paramInfoMap;
  IntParam a_number;

  private:
  std::map<std::string, ParamInfo> paramInfoMapInit();
};
} // namespace tiny