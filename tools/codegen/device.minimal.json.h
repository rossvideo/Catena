#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace minimal {
struct Location {
  struct Latitude {
    int32_t degrees{1};
    int32_t minutes{2};
    int32_t seconds{3};
    using isCatenaStruct = void;
  };
  Latitude latitude{.degrees{4},.minutes{5},.seconds{6}};
  struct Longitude {
    int32_t degrees{7};
    int32_t minutes{8};
    int32_t seconds{9};
    using isCatenaStruct = void;
  };
  Longitude longitude{.degrees{10},.minutes{11},.seconds{12}};
  using isCatenaStruct = void;
};
} // namespace minimal
template<>
struct catena::common::StructInfo<minimal::Location::Latitude> {
  using Latitude = minimal::Location::Latitude;
  using Type = std::tuple<FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>>;
  static constexpr Type fields = {{"degrees", &Latitude::degrees}, {"minutes", &Latitude::minutes}, {"seconds", &Latitude::seconds}};
};
template<>
struct catena::common::StructInfo<minimal::Location::Longitude> {
  using Longitude = minimal::Location::Longitude;
  using Type = std::tuple<FieldInfo<int32_t, Longitude>, FieldInfo<int32_t, Longitude>, FieldInfo<int32_t, Longitude>>;
  static constexpr Type fields = {{"degrees", &Longitude::degrees}, {"minutes", &Longitude::minutes}, {"seconds", &Longitude::seconds}};
};
template<>
struct catena::common::StructInfo<minimal::Location> {
  using Location = minimal::Location;
  using Type = std::tuple<FieldInfo<minimal::Location::Latitude, Location>, FieldInfo<minimal::Location::Longitude, Location>>;
  static constexpr Type fields = {{"latitude", &Location::latitude}, {"longitude", &Location::longitude}};
};
