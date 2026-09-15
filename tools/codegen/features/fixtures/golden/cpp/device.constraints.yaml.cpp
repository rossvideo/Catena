// This file was auto-generated. Do not modify by hand.
#include "device.constraints.yaml.h"
using namespace constraints;
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
catena::common::RangeConstraint<int32_t> shared_sexagesimal(0, 59, 1, 0, 59, "sexagesimal", true, dm);
Product product{.name{"Cucumber Fixture"},.vendor{"Ross Video"},.version{"1.0.0"},.serial_number{"SN-7K9M-2024-XR485-BLU"}};
catena::common::ParamDescriptor _productDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:mon", true, false, "product", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _product_nameDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "name", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_vendorDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "vendor", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_catena_sdkDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "catena_sdk", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_catena_sdk_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "catena_sdk_version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_serial_numberDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "serial_number", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamWithValue<constraints::Product> _productParam(product, _productDescriptor, dm, false);
int32_t sexagesimal{0};
catena::common::ParamDescriptor _sexagesimalDescriptor(st2138::ParamType::INT32, {}, {{"en", "Seconds"}}, "number", "", false, false, "sexagesimal", "", &shared_sexagesimal, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<int32_t> _sexagesimalParam(sexagesimal, _sexagesimalDescriptor, dm, false);
int32_t power{0};
catena::common::ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE> _powerConstraint({{0,{{"en","Off"}}},{1,{{"en","On"}}}}, false, "power", false);
catena::common::ParamDescriptor _powerDescriptor(st2138::ParamType::INT32, {}, {{"en", "Power"}}, "button", "", false, false, "power", "", &_powerConstraint, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<int32_t> _powerParam(power, _powerDescriptor, dm, false);
std::vector<int32_t> odd_numbers{1, 3, 5, 7, 9};
catena::common::RangeConstraint<int32_t> _odd_numbersConstraint(1, 9, 2, 1, 9, "odd_numbers", false);
catena::common::ParamDescriptor _odd_numbersDescriptor(st2138::ParamType::INT32_ARRAY, {}, {{"en", "Odd Numbers"}}, "number", "", false, false, "odd_numbers", "", &_odd_numbersConstraint, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::vector<int32_t>> _odd_numbersParam(odd_numbers, _odd_numbersDescriptor, dm, false);
float gain{0};
catena::common::RangeConstraint<float> _gainConstraint(-2, 2, 0.25, -2, 2, "gain", false);
catena::common::ParamDescriptor _gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Gain"}}, "fader", "", false, false, "gain", "", &_gainConstraint, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<float> _gainParam(gain, _gainDescriptor, dm, false);
std::string channel{"a"};
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE> _channelConstraint({{"a",{}},{"b",{}},{"c",{}}}, true, "channel", false);
catena::common::ParamDescriptor _channelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Channel"}}, "", "", false, false, "channel", "", &_channelConstraint, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _channelParam(channel, _channelDescriptor, dm, false);
std::string state{"stopped"};
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE> _stateConstraint({{"stopped",{{"en","Stopped"}}},{"playing",{{"en","Playing"}}}}, true, "state", false);
catena::common::ParamDescriptor _stateDescriptor(st2138::ParamType::STRING, {}, {{"en", "State"}}, "", "", false, false, "state", "", &_stateConstraint, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _stateParam(state, _stateDescriptor, dm, false);
constexpr const char* real_sdk_version = CATENA_CPP_VERSION;
constexpr const char* real_sdk_url = CATENA_CPP_SDK;
constraints::Product& initialize_sdk_version(constraints::Product& p) {
  p.catena_sdk = real_sdk_url;
  p.catena_sdk_version = real_sdk_version;
  return p;
}
constraints::Product dummy = initialize_sdk_version(product);
