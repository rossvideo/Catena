// This file was auto-generated. Do not modify by hand.
#include "device.minimal.json.h"
using namespace minimal;
#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <LanguagePack.h>
#include <Device.h>
#include <RangeConstraint.h>
#include <ChoiceConstraint.h>
#include <Enums.h>
#include <StructInfo.h>
#include <string>
#include <vector>
#include <functional>
#include <Menu.h>
#include <MenuGroup.h>
using catena::Device_DetailLevel;
using DetailLevel = catena::common::DetailLevel;
using catena::common::Scopes_e;
using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;
using catena::common::FieldInfo;
using catena::common::ParamDescriptor;
using catena::common::ParamWithValue;
using catena::common::Device;
using catena::common::RangeConstraint;
using catena::common::ChoiceConstraint;
using catena::common::IParam;
using catena::common::EmptyValue;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::common::Device dm {1, DetailLevel("FULL")(), {"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"}, "st2138:op", true, false};

using catena::common::LanguagePack;
LanguagePack es {
  "es",
  "Spanish",
  {
    { "greeting", "Hola" },
    { "parting", "Adiós" }
  },
  dm
};
LanguagePack en {
  "en",
  "English",
  {
    { "greeting", "Hello" },
    { "parting", "Goodbye" }
  },
  dm
};
LanguagePack fr {
  "fr",
  "French",
  {
    { "greeting", "Bonjour" },
    { "parting", "Adieu" }
  },
  dm
};
using catena::common::Menu;
using catena::common::MenuGroup;
catena::common::ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> shared_shared_string_constraint({{"A",{}},{"B",{}},{"C",{}}}, false, "shared_string_constraint", true, dm);
catena::common::ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> shared_shared_int32_constraint({{1,{{"en","one"}}},{2,{{"en","two"}}},{3,{{"en","three"}}}}, false, "shared_int32_constraint", true, dm);
catena::common::RangeConstraint<float> shared_shared_float32_constraint(0, 100, 0, 0, 100, "shared_float32_constraint", true, dm);
Location location{.latitude{.degrees{13},.minutes{14},.seconds{15}},.longitude{.degrees{16},.minutes{17},.seconds{18}}};
catena::common::ParamDescriptor _locationDescriptor(catena::ParamType::STRUCT, {}, {{"en", "Location"}}, "", "", false, "location", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamDescriptor _location_latitudeDescriptor(catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, "", "", false, "latitude", "", nullptr, false, false, dm, 0, 0, 2, false, &_locationDescriptor);
catena::common::ParamDescriptor _location_latitude_degreesDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "degrees", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_latitudeDescriptor);
catena::common::ParamDescriptor _location_latitude_minutesDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "minutes", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_latitudeDescriptor);
catena::common::ParamDescriptor _location_latitude_secondsDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "seconds", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_latitudeDescriptor);
catena::common::ParamDescriptor _location_longitudeDescriptor(catena::ParamType::STRUCT, {}, {{"en", "Latitude"}}, "", "", false, "longitude", "", nullptr, false, false, dm, 0, 0, 2, false, &_locationDescriptor);
catena::common::ParamDescriptor _location_longitude_degreesDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "degrees", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_longitudeDescriptor);
catena::common::ParamDescriptor _location_longitude_minutesDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "minutes", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_longitudeDescriptor);
catena::common::ParamDescriptor _location_longitude_secondsDescriptor(catena::ParamType::INT32, {}, {}, "", "", false, "seconds", "", nullptr, false, false, dm, 0, 0, 2, false, &_location_longitudeDescriptor);
catena::common::ParamWithValue<minimal::Location> _locationParam(location, _locationDescriptor, dm, false);
