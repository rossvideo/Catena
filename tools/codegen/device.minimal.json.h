#pragma once
// This file was auto-generated. Do not modify by hand.
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>
using catena::lite::StructInfo;
extern catena::lite::Device dm;
namespace minimal {
struct Location {
  struct Latitude {
    int32_t degrees;
    int32_t minutes;
    int32_t seconds;
    static const StructInfo& getStructInfo();
  };
  Latitude latitude;
  struct Longitude {
    int32_t degrees;
    int32_t minutes;
    int32_t seconds;
    static const StructInfo& getStructInfo();
  };
  Longitude longitude;
  static const StructInfo& getStructInfo();
};
} // namespace minimal
