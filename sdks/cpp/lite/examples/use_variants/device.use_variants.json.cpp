// This file was auto-generated. Do not modify by hand.
#include "device.use_variants.json.h"
using namespace use_variants;
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
using catena::lite::EmptyValue;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::lite::Device dm {1, DetailLevel("FULL")(), {Scope("monitor")(), Scope("operate")(), Scope("configure")(), Scope("administer")()}, Scope("operate")(), true, false};

using catena::lite::LanguagePack;
use_variants::Location location {{1,2,3},{4,5,6}};
catena::lite::ParamDescriptor _locationDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Location"}}, {}, "", false, "location", nullptr, nullptr, dm, false};
catena::lite::ParamDescriptor _location_latitudeDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, {}, "", false, "latitude", nullptr, &_locationDescriptor, dm, false};
catena::lite::ParamDescriptor _location_latitude_degreesDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", nullptr, &_location_latitudeDescriptor, dm, false};
catena::lite::ParamDescriptor _location_latitude_minutesDescriptor {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", nullptr, &_location_latitudeDescriptor, dm, false};
catena::lite::ParamDescriptor _location_latitude_secondsDescriptor {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", nullptr, &_location_latitudeDescriptor, dm, false};
catena::lite::ParamDescriptor _location_longitudeDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Longitude"}}, {}, "", false, "longitude", nullptr, &_locationDescriptor, dm, false};
catena::lite::ParamDescriptor _location_longitude_degreesDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Degrees"}}, {}, "", false, "degrees", nullptr, &_location_longitudeDescriptor, dm, false};
catena::lite::ParamDescriptor _location_longitude_minutesDescriptor {catena::ParamType::INT32, {}, {{"en", "Minutes"}}, {}, "", false, "minutes", nullptr, &_location_longitudeDescriptor, dm, false};
catena::lite::ParamDescriptor _location_longitude_secondsDescriptor {catena::ParamType::INT32, {}, {{"en", "Seconds"}}, {}, "", false, "seconds", nullptr, &_location_longitudeDescriptor, dm, false};
catena::lite::ParamWithValue<use_variants::Location> _locationParam {location, _locationDescriptor, dm, false};
