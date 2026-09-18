// This file was auto-generated. Do not modify by hand.
#include "device.namespaces.yaml.h"
using namespace namespaces;
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
using st2138::Device_DetailLevel;
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
using catena::common::Menu;
using catena::common::MenuGroup;
Product product{.name{"Cucumber Fixture"},.vendor{"Ross Video"},.version{"1.0.0"},.serial_number{"SN-7K9M-2024-XR485-BLU"}};
catena::common::ParamDescriptor _productDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:mon", true, false, "product", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _product_nameDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "name", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_vendorDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "vendor", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_catena_sdkDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "catena_sdk", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_catena_sdk_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "catena_sdk_version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_serial_numberDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "serial_number", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamWithValue<namespaces::Product> _productParam(product, _productDescriptor, dm, false);
shared::geo::Segment flight_path{.start{.latitude{45.4215},.longitude{-75.6972}},.end{.latitude{43.6532},.longitude{-79.3832}}};
catena::common::ParamDescriptor _flight_pathDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Flight Path"}}, "", "", false, false, "flight_path", "geo_lib/segment", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _flight_path_startDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Point"}}, "", "", false, false, "start", "geo_lib/point", nullptr, false, false, dm, 0, 0, 0, false, &_flight_pathDescriptor);
catena::common::RangeConstraint<float> _geo_lib_point_latitudeConstraint(-90, 90, 0, -90, 90, "latitude", false);
catena::common::ParamDescriptor _flight_path_start_latitudeDescriptor(st2138::ParamType::FLOAT32, {}, {}, "", "", false, false, "latitude", "", &_geo_lib_point_latitudeConstraint, false, false, dm, 0, 0, 2, false, &_flight_path_startDescriptor);
catena::common::RangeConstraint<float> _geo_lib_point_longitudeConstraint(-180, 180, 0, -180, 180, "longitude", false);
catena::common::ParamDescriptor _flight_path_start_longitudeDescriptor(st2138::ParamType::FLOAT32, {}, {}, "", "", false, false, "longitude", "", &_geo_lib_point_longitudeConstraint, false, false, dm, 0, 0, 2, false, &_flight_path_startDescriptor);
catena::common::ParamDescriptor _flight_path_endDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Point"}}, "", "", false, false, "end", "geo_lib/point", nullptr, false, false, dm, 0, 0, 0, false, &_flight_pathDescriptor);
catena::common::ParamDescriptor _flight_path_end_latitudeDescriptor(st2138::ParamType::FLOAT32, {}, {}, "", "", false, false, "latitude", "", &_geo_lib_point_latitudeConstraint, false, false, dm, 0, 0, 2, false, &_flight_path_endDescriptor);
catena::common::ParamDescriptor _flight_path_end_longitudeDescriptor(st2138::ParamType::FLOAT32, {}, {}, "", "", false, false, "longitude", "", &_geo_lib_point_longitudeConstraint, false, false, dm, 0, 0, 2, false, &_flight_path_endDescriptor);
catena::common::ParamWithValue<shared::geo::Segment> _flight_pathParam(flight_path, _flight_pathDescriptor, dm, false);
constexpr const char* real_sdk_version = CATENA_CPP_VERSION;
constexpr const char* real_sdk_url = CATENA_CPP_SDK;
namespaces::Product& initialize_sdk_version(namespaces::Product& p) {
  p.catena_sdk = real_sdk_url;
  p.catena_sdk_version = real_sdk_version;
  return p;
}
namespaces::Product dummy = initialize_sdk_version(product);
