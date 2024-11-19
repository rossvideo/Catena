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
using catena::common::FieldInfo;
using catena::common::ParamDescriptor;
using catena::common::ParamWithValue;
using catena::common::Device;
using catena::common::RangeConstraint;
using catena::common::PicklistConstraint;
using catena::common::NamedChoiceConstraint;
using catena::common::IParam;
using catena::common::EmptyValue;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::common::Device dm {1, DetailLevel("FULL")(), {Scope("monitor")(), Scope("operate")(), Scope("configure")(), Scope("administer")()}, Scope("operate")(), true, false};

using catena::common::LanguagePack;
use_variants::Number number {42};
catena::common::ParamDescriptor _numberDescriptor {catena::ParamType::STRUCT_VARIANT, {}, {{"en", "Number"}}, {}, "", false, "number", nullptr, nullptr, dm, false};
catena::common::ParamDescriptor _number_wordsDescriptor {catena::ParamType::STRING, {}, {}, {}, "", false, "words", nullptr, &_numberDescriptor, dm, false};
catena::common::ParamDescriptor _number_digitsDescriptor {catena::ParamType::INT32, {}, {}, {}, "", false, "digits", nullptr, &_numberDescriptor, dm, false};
catena::common::ParamWithValue<use_variants::Number> _numberParam {number, _numberDescriptor, dm, false};
use_variants::Cartesian cartesian {5,10,15};
catena::common::ParamDescriptor _cartesianDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Cartesian"}}, {}, "", false, "cartesian", nullptr, nullptr, dm, false};
catena::common::ParamDescriptor _cartesian_xDescriptor {catena::ParamType::INT32, {}, {{"en", "X"}}, {}, "", false, "x", nullptr, &_cartesianDescriptor, dm, false};
catena::common::ParamDescriptor _cartesian_yDescriptor {catena::ParamType::INT32, {}, {{"en", "Y"}}, {}, "", false, "y", nullptr, &_cartesianDescriptor, dm, false};
catena::common::ParamDescriptor _cartesian_zDescriptor {catena::ParamType::INT32, {}, {{"en", "Z"}}, {}, "", false, "z", nullptr, &_cartesianDescriptor, dm, false};
catena::common::ParamWithValue<use_variants::Cartesian> _cartesianParam {cartesian, _cartesianDescriptor, dm, false};
use_variants::Coordinates coordinates {Cartesian{1,2,3},_coordinates::Cylindrical{4,45,6},_coordinates::Spherical{7,90,180}};
catena::common::ParamDescriptor _coordinatesDescriptor {catena::ParamType::STRUCT_VARIANT_ARRAY, {}, {{"en", "Coordinates"}}, {}, "", false, "coordinates", nullptr, nullptr, dm, false};
catena::common::ParamDescriptor _coordinates_cartesianDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Cartesian"}}, {}, "", false, "cartesian", nullptr, &_coordinatesDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cartesian_xDescriptor {catena::ParamType::INT32, {}, {{"en", "X"}}, {}, "", false, "x", nullptr, &_coordinates_cartesianDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cartesian_yDescriptor {catena::ParamType::INT32, {}, {{"en", "Y"}}, {}, "", false, "y", nullptr, &_coordinates_cartesianDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cartesian_zDescriptor {catena::ParamType::INT32, {}, {{"en", "Z"}}, {}, "", false, "z", nullptr, &_coordinates_cartesianDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cylindricalDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Cylindrical"}}, {}, "", false, "cylindrical", nullptr, &_coordinatesDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cylindrical_rhoDescriptor {catena::ParamType::INT32, {}, {{"en", "Rho"}}, {}, "", false, "rho", nullptr, &_coordinates_cylindricalDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cylindrical_phiDescriptor {catena::ParamType::INT32, {}, {{"en", "Phi"}}, {}, "", false, "phi", nullptr, &_coordinates_cylindricalDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_cylindrical_zDescriptor {catena::ParamType::INT32, {}, {{"en", "Z"}}, {}, "", false, "z", nullptr, &_coordinates_cylindricalDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_sphericalDescriptor {catena::ParamType::STRUCT, {}, {{"en", "Spherical"}}, {}, "", false, "spherical", nullptr, &_coordinatesDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_spherical_rDescriptor {catena::ParamType::INT32, {}, {{"en", "R"}}, {}, "", false, "r", nullptr, &_coordinates_sphericalDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_spherical_thetaDescriptor {catena::ParamType::INT32, {}, {{"en", "Theta"}}, {}, "", false, "theta", nullptr, &_coordinates_sphericalDescriptor, dm, false};
catena::common::ParamDescriptor _coordinates_spherical_phiDescriptor {catena::ParamType::INT32, {}, {{"en", "Phi"}}, {}, "", false, "phi", nullptr, &_coordinates_sphericalDescriptor, dm, false};
catena::common::ParamWithValue<use_variants::Coordinates> _coordinatesParam {coordinates, _coordinatesDescriptor, dm, false};
