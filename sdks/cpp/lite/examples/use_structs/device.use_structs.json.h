#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace use_structs {
// struct Location {
//   struct Latitude {
//     int32_t degrees;
//     int32_t minutes;
//     int32_t seconds;
//     static const catena::lite::StructInfo& getStructInfo();
//   };
//   Latitude latitude;
//   struct Longitude {
//     int32_t degrees;
//     int32_t minutes;
//     int32_t seconds;
//     static const catena::lite::StructInfo& getStructInfo();
//   };
//   Longitude longitude;
//   static const catena::lite::StructInfo& getStructInfo();
// };
struct Latitude {
    int32_t degrees;
    int32_t minutes;
    int32_t seconds;
    static const catena::lite::StructInfo& getStructInfo();
  };
} // namespace use_structs
