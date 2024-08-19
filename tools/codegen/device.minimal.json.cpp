// This file was auto-generated. Do not modify by hand.
#include "device.minimal.json.h"
#include <lite/include/ParamDescriptor.h>
#include <lite/include/ParamWithValue.h>
#include <lite/include/Device.h>
#include <common/include/Enums.h>
#include <lite/include/StructInfo.h>
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
using catena::lite::IParam;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::lite::Device dm {1, DetailLevel("FULL")(), {Scope("monitor")(), Scope("operate")(), Scope("configure")(), Scope("administer")()}, Scope("operate")(), false, false};

minimal::Location location {{0,0,0},{0,0,0}};
catena::lite::ParamWithValue<minimal::Location> _locationParam {catena::ParamType::STRUCT, {}, {{"en", "Location"}}, {}, false, "location", dm, location};
catena::lite::ParamDescriptor<minimal::Location::Latitude> _location_latitudeParam {catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, {}, false, "latitude", &_locationParam};
catena::lite::ParamDescriptor<int32_t> _location_latitude_degreesParam {catena::ParamType::INT32, {}, {}, {}, false, "degrees", &_location_latitudeParam};
catena::lite::ParamDescriptor<int32_t> _location_latitude_minutesParam {catena::ParamType::INT32, {}, {}, {}, false, "minutes", &_location_latitudeParam};
catena::lite::ParamDescriptor<int32_t> _location_latitude_secondsParam {catena::ParamType::INT32, {}, {}, {}, false, "seconds", &_location_latitudeParam};
catena::lite::ParamDescriptor<minimal::Location::Longitude> _location_longitudeParam {catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, {}, false, "longitude", &_locationParam};
catena::lite::ParamDescriptor<int32_t> _location_longitude_degreesParam {catena::ParamType::INT32, {}, {}, {}, false, "degrees", &_location_longitudeParam};
catena::lite::ParamDescriptor<int32_t> _location_longitude_minutesParam {catena::ParamType::INT32, {}, {}, {}, false, "minutes", &_location_longitudeParam};
catena::lite::ParamDescriptor<int32_t> _location_longitude_secondsParam {catena::ParamType::INT32, {}, {}, {}, false, "seconds", &_location_longitudeParam};
const StructInfo& minimal::Location::getStructInfo() {
  static StructInfo si {
    "Location",
    {
      { "latitude", offsetof(Location, latitude)},
      { "longitude", offsetof(Location, longitude)}
    }
  };
  return si;
}
const StructInfo& minimal::Location::Latitude::getStructInfo() {
  static StructInfo si {
    "Latitude",
    {
      { "degrees", offsetof(Latitude, degrees)},
      { "minutes", offsetof(Latitude, minutes)},
      { "seconds", offsetof(Latitude, seconds)}
    }
  };
  return si;
}
const StructInfo& minimal::Location::Longitude::getStructInfo() {
  static StructInfo si {
    "Longitude",
    {
      { "degrees", offsetof(Longitude, degrees)},
      { "minutes", offsetof(Longitude, minutes)},
      { "seconds", offsetof(Longitude, seconds)}
    }
  };
  return si;
}
