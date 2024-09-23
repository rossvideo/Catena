// This file was auto-generated. Do not modify by hand.
#include "device.use_structs.json.h"
using namespace use_structs;
#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <LanguagePack.h>
#include <Device.h>
#include <RangeConstraint.h>
#include <PicklistConstraint.h>
#include <NamedChoiceConstraint.h>
#include <Enums.h>
#include <StructInfo.h>
#include <string>
#include <vector>
#include <functional>
using catena::Device_DetailLevel;
using DetailLevel = catena::common::DetailLevel;
using catena::common::Scopes_e;
using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;
using catena::lite::FieldInfo;
using catena::lite::ParamDescriptor;
using catena::lite::ParamWithValue;
using catena::lite::Device;
using catena::lite::RangeConstraint;
using catena::lite::PicklistConstraint;
using catena::lite::NamedChoiceConstraint;
using catena::common::IParam;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::lite::Device dm {1, DetailLevel("FULL")(), {Scope("monitor")(), Scope("operate")(), Scope("configure")(), Scope("administer")()}, Scope("operate")(), false, false};

using catena::lite::LanguagePack;
use_structs::Location location {{1.0f,2,3},{4.0f,5,6}};
catena::lite::ParamDescriptor _locationDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Location"}}, {}, "", false, "location", nullptr, dm};
catena::lite::ParamDescriptor _location_latitudeDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, {}, "", false, "latitude", &_locationDescriptor, dm};
catena::lite::ParamDescriptor _location_latitude_degreesDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", &_location_latitudeDescriptor, dm};
catena::lite::ParamDescriptor _location_latitude_minutesDescriptor {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", &_location_latitudeDescriptor, dm};
catena::lite::ParamDescriptor _location_latitude_secondsDescriptor {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", &_location_latitudeDescriptor, dm};
catena::lite::ParamDescriptor _location_longitudeDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Longitude"}}, {}, "", false, "longitude", &_locationDescriptor, dm};
catena::lite::ParamDescriptor _location_longitude_degreesDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", &_location_longitudeDescriptor, dm};
catena::lite::ParamDescriptor _location_longitude_minutesDescriptor {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", &_location_longitudeDescriptor, dm};
catena::lite::ParamDescriptor _location_longitude_secondsDescriptor {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", &_location_longitudeDescriptor, dm};
catena::lite::ParamWithValue<use_structs::Location> _locationParam {location, _locationDescriptor, dm};
template <>
struct catena::lite::StructInfo<use_structs::Location> {
  using Location = use_structs::Location;
  using Type = std::tuple<FieldInfo<Location::Latitude, Location>, FieldInfo<Location::Longitude, Location>>;
  static constexpr Type fields = {{"latitude", &Location::latitude}, {"longitude", &Location::longitude}};
};
template<>
struct catena::lite::StructInfo<use_structs::Location::Latitude> {
  using Latitude = use_structs::Location::Latitude;
  using Type = std::tuple<FieldInfo<float, Latitude>, FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>>;
  static constexpr Type fields = {{"degrees", &Latitude::degrees}, {"minutes", &Latitude::minutes}, {"seconds", &Latitude::seconds}};
};
template<>
struct catena::lite::StructInfo<use_structs::Location::Longitude> {
  using Longitude = use_structs::Location::Longitude;
  using Type = std::tuple<FieldInfo<float, Longitude>, FieldInfo<int32_t, Longitude>, FieldInfo<int32_t, Longitude>>;
  static constexpr Type fields = {{"degrees", &Longitude::degrees}, {"minutes", &Longitude::minutes}, {"seconds", &Longitude::seconds}};
};


// use_structs::Latitude latitude {1.0f,2,3};
// catena::lite::ParamDescriptor _latitudeDescriptor {catena::ParamType::STRUCT, {}, {{"en", "latitude"}}, {}, "", false, "latitude", nullptr, dm};
// catena::lite::ParamDescriptor _latitude_degreesDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", &_latitudeDescriptor, dm};
// catena::lite::ParamDescriptor _latitude_minutesDescriptor {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", &_latitudeDescriptor, dm};
// catena::lite::ParamDescriptor _latitude_secondsDescriptor {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", &_latitudeDescriptor, dm};
// catena::lite::ParamWithValue<use_structs::Latitude> _latitudeParam {latitude, _latitudeDescriptor, dm};
// template<>
// struct catena::lite::StructInfo<use_structs::Latitude> {
//   using Latitude = use_structs::Latitude;
//   using Type = std::tuple<FieldInfo<float, Latitude>, FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>>;
//   static constexpr Type fields = {{"degrees", &Latitude::degrees}, {"minutes", &Latitude::minutes}, {"seconds", &Latitude::seconds}};
// };