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
using catena::lite::StructInfo;
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
// use_structs::Location location {{1,2,3},{4,5,6}};
// catena::lite::ParamWithValue<use_structs::Location> _locationParam {catena::ParamType::STRUCT, {}, {{"en", "Location"}}, {}, "", false, "location", dm, location};
// catena::lite::ParamDescriptor<use_structs::Location::Latitude> _location_latitudeParam {catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, {}, "", false, "latitude", &_locationParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_latitude_degreesParam {catena::ParamType::INT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", &_location_latitudeParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_latitude_minutesParam {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", &_location_latitudeParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_latitude_secondsParam {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", &_location_latitudeParam, dm};
// catena::lite::ParamDescriptor<use_structs::Location::Longitude> _location_longitudeParam {catena::ParamType::STRUCT, {}, {{"en", "Longitude"}}, {}, "", false, "longitude", &_locationParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_longitude_degreesParam {catena::ParamType::INT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", &_location_longitudeParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_longitude_minutesParam {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", &_location_longitudeParam, dm};
// catena::lite::ParamDescriptor<int32_t> _location_longitude_secondsParam {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", &_location_longitudeParam, dm};
// const StructInfo& use_structs::Location::getStructInfo() {
//   static StructInfo si {
//     "Location",
//     {
//       { "latitude", offsetof(Location, latitude)},
//       { "longitude", offsetof(Location, longitude)}
//     }
//   };
//   return si;
// }
// template <>
// struct catena::lite::Reflect<use_structs::Location> {
//   using Location = use_structs::Location;
//   using Type = std::tuple<Location::Latitude Location::*, Location::Longitude Location::*>;
//   static constexpr Type fields = {&Location::latitude, &Location::longitude};
// };
// const StructInfo& use_structs::Location::Latitude::getStructInfo() {
//   static StructInfo si {
//     "Latitude",
//     {
//       { "degrees", offsetof(Latitude, degrees)},
//       { "minutes", offsetof(Latitude, minutes)},
//       { "seconds", offsetof(Latitude, seconds)}
//     }
//   };
//   return si;
// }
// template<>
// struct catena::lite::Reflect<use_structs::Location::Latitude> {
//   using Latitude = use_structs::Location::Latitude;
//   using Type = std::tuple<int32_t Latitude::*, int32_t Latitude::*, int32_t Latitude::*>;
//   static constexpr Type fields = {&Latitude::degrees, &Latitude::minutes, &Latitude::seconds};
// };
// const StructInfo& use_structs::Location::Longitude::getStructInfo() {
//   static StructInfo si {
//     "Longitude",
//     {
//       { "degrees", offsetof(Longitude, degrees)},
//       { "minutes", offsetof(Longitude, minutes)},
//       { "seconds", offsetof(Longitude, seconds)}
//     }
//   };
//   return si;
// }
// template<>
// struct catena::lite::Reflect<use_structs::Location::Longitude> {
//   using Longitude = use_structs::Location::Longitude;
//   using Type = std::tuple<int32_t Longitude::*, int32_t Longitude::*, int32_t Longitude::*>;
//   static constexpr Type fields = {&Longitude::degrees, &Longitude::minutes, &Longitude::seconds};
// };
use_structs::Latitude latitude {1,2,3};
catena::lite::ParamWithValue<use_structs::Latitude> _latitudeParam {catena::ParamType::STRUCT, {}, {{"en", "latitude"}}, {}, "", false, "latitude", dm, latitude};
const StructInfo& use_structs::Latitude::getStructInfo() {
  static StructInfo si {
    "Latitude",
    // {
    //   { "degrees", offsetof(Latitude, degrees)},
    //   { "minutes", offsetof(Latitude, minutes)},
    //   { "seconds", offsetof(Latitude, seconds)}
    // }
  };
  return si;
}
template<>
struct catena::lite::Reflect<use_structs::Latitude> {
  using Latitude = use_structs::Latitude;
  using Type = std::tuple<FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>, FieldInfo<int32_t, Latitude>>;
  static constexpr Type fields = {{"degrees", &Latitude::degrees}, {"minutes", &Latitude::minutes}, {"seconds", &Latitude::seconds}};
};