#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace use_structs {
struct Location {
  struct Latitude {
    float degrees;
    int32_t minutes;
    int32_t seconds;
    static void isCatenaStruct() {};
  };
  Latitude latitude;
  struct Longitude {
    float degrees;
    int32_t minutes;
    int32_t seconds;
    static void isCatenaStruct() {};
  };
  Longitude longitude;
  static void isCatenaStruct() {};
};
// struct Latitude {
//     int32_t degrees;
//     int32_t minutes;
//     int32_t seconds;
//     static void isCatenaStruct() {};
//   };
} // namespace use_structs
