// This file was auto-generated. Do not modify by hand.
#include "device.menus.yaml.h"
using namespace menus;
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
MenuGroup _statusGroup {
  "status", 0,
  {
    { "en", "Status" },
    { "fr", "Statut" }
  },
  dm
};
Menu _statusGroup_productMenu {
  { { "en", "Product" }, { "fr", "Produit" } },
  false, false,
  { "product/name", "product/vendor", "product/version" },
  {  },
  {  }, "product", 0, _statusGroup
};
MenuGroup _configGroup {
  "config", 1,
  {
    { "en", "Config" },
    { "fr", "Configuration" }
  },
  dm
};
Menu _configGroup_generalMenu {
  { { "en", "General" }, { "fr", "Général" } },
  false, false,
  { "counter", "label" },
  {  },
  { { "oglml", "eo://menus.grid" } }, "general", 0, _configGroup
};
Product product{.name{"Cucumber Fixture"},.vendor{"Ross Video"},.version{"1.0.0"},.serial_number{"SN-7K9M-2024-XR485-BLU"}};
catena::common::ParamDescriptor _productDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:mon", true, false, "product", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _product_nameDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "name", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_vendorDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "vendor", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdkDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdk_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk_version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_serial_numberDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "serial_number", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamWithValue<menus::Product> _productParam(product, _productDescriptor, dm, false);
int32_t counter{0};
catena::common::ParamDescriptor _counterDescriptor(st2138::ParamType::INT32, {}, {{"en", "Counter"}}, "", "", false, false, "counter", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<int32_t> _counterParam(counter, _counterDescriptor, dm, false);
std::string label{"hello"};
catena::common::ParamDescriptor _labelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Label"}}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _labelParam(label, _labelDescriptor, dm, false);
constexpr const char* real_sdk_version = CATENA_CPP_VERSION;
constexpr const char* real_sdk_url = CATENA_CPP_SDK;
menus::Product& initialize_sdk_version(menus::Product& p) {
  p.st2138_sdk = real_sdk_url;
  p.st2138_sdk_version = real_sdk_version;
  return p;
}
menus::Product dummy = initialize_sdk_version(product);
