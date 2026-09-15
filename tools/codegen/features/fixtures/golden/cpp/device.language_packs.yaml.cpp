// This file was auto-generated. Do not modify by hand.
#include "device.language_packs.yaml.h"
using namespace language_packs;
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
LanguagePack en {
  "en",
  "English",
  {
    { "greeting", "Hello" },
    { "parting", "Goodbye" }
  },
  dm
};
LanguagePack es {
  "es",
  "Spanish",
  {
    { "greeting", "Hola" },
    { "parting", "Adiós" }
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
LanguagePack de {
  "de",
  "German",
  {
    { "greeting", "Hallo" },
    { "parting", "Auf Wiedersehen" }
  },
  dm
};
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
catena::common::ParamWithValue<language_packs::Product> _productParam(product, _productDescriptor, dm, false);
std::string greeting{"Hello, World!"};
catena::common::ParamDescriptor _greetingDescriptor(st2138::ParamType::STRING, {}, {{"$key", "greeting"}}, "", "", false, false, "greeting", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _greetingParam(greeting, _greetingDescriptor, dm, false);
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
constexpr const char* real_sdk_version = TO_STRING(CATENA_CPP_VERSION);
language_packs::Product& initialize_sdk_version(language_packs::Product& p) {
  p.catena_sdk_version = real_sdk_version;
  return p;
}
language_packs::Product dummy = initialize_sdk_version(product);
