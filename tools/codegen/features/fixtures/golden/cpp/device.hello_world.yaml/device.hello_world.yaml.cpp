// This file was auto-generated. Do not modify by hand.
#include "device.hello_world.yaml.h"
using namespace hello_world;
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
Product product{.name{"Cucumber Fixture"},.vendor{"Ross Video"},.version{"1.0.0"},.serial_number{"SN-7K9M-2024-XR485-BLU"}};
catena::common::ParamDescriptor _productDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:mon", true, false, "product", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _product_nameDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "name", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_vendorDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "vendor", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdkDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdk_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk_version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_serial_numberDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "serial_number", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamWithValue<hello_world::Product> _productParam(product, _productDescriptor, dm, false);
std::string hello{"Hello, World!"};
catena::common::ParamDescriptor _helloDescriptor(st2138::ParamType::STRING, {}, {{"$key", "greeting"}}, "", "", false, false, "hello", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _helloParam(hello, _helloDescriptor, dm, false);
int32_t count{1234};
catena::common::ParamDescriptor _countDescriptor(st2138::ParamType::INT32, {}, {{"en", "Counter"}}, "", "", false, false, "count", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<int32_t> _countParam(count, _countDescriptor, dm, false);
float gain{0.707};
catena::common::ParamDescriptor _gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Audio Gain"}}, "fader", "", false, false, "gain", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<float> _gainParam(gain, _gainDescriptor, dm, false);
std::vector<int32_t> primes{2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
catena::common::ParamDescriptor _primesDescriptor(st2138::ParamType::INT32_ARRAY, {}, {{"en", "The First Few Primes"}}, "", "", false, false, "primes", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::vector<int32_t>> _primesParam(primes, _primesDescriptor, dm, false);
constexpr const char* real_sdk_version = CATENA_CPP_VERSION;
constexpr const char* real_sdk_url = CATENA_CPP_SDK;
hello_world::Product& initialize_sdk_version(hello_world::Product& p) {
  p.st2138_sdk = real_sdk_url;
  p.st2138_sdk_version = real_sdk_version;
  return p;
}
hello_world::Product dummy = initialize_sdk_version(product);
