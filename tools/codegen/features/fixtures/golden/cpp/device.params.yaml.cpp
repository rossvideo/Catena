// This file was auto-generated. Do not modify by hand.
#include "device.params.yaml.h"
using namespace params;
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
catena::common::ParamWithValue<params::Product> _productParam(product, _productDescriptor, dm, false);
int32_t int32_scalar{42};
catena::common::ParamDescriptor _int32_scalarDescriptor(st2138::ParamType::INT32, {}, {{"en", "Int32"}}, "", "", false, false, "int32_scalar", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<int32_t> _int32_scalarParam(int32_scalar, _int32_scalarDescriptor, dm, false);
float float32_scalar{0.707};
catena::common::ParamDescriptor _float32_scalarDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Float32"}}, "", "", false, false, "float32_scalar", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<float> _float32_scalarParam(float32_scalar, _float32_scalarDescriptor, dm, false);
std::string string_scalar{"hello"};
catena::common::ParamDescriptor _string_scalarDescriptor(st2138::ParamType::STRING, {}, {{"en", "String"}}, "", "", false, false, "string_scalar", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::string> _string_scalarParam(string_scalar, _string_scalarDescriptor, dm, false);
std::vector<int32_t> int32_array{1, 2, 3};
catena::common::ParamDescriptor _int32_arrayDescriptor(st2138::ParamType::INT32_ARRAY, {}, {{"en", "Int32 Array"}}, "", "", false, false, "int32_array", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::vector<int32_t>> _int32_arrayParam(int32_array, _int32_arrayDescriptor, dm, false);
std::vector<float> float32_array{1.1, 2.2, 3.3};
catena::common::ParamDescriptor _float32_arrayDescriptor(st2138::ParamType::FLOAT32_ARRAY, {}, {{"en", "Float32 Array"}}, "", "", false, false, "float32_array", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::vector<float>> _float32_arrayParam(float32_array, _float32_arrayDescriptor, dm, false);
std::vector<std::string> string_array{"a", "b", "c"};
catena::common::ParamDescriptor _string_arrayDescriptor(st2138::ParamType::STRING_ARRAY, {}, {{"en", "String Array"}}, "", "", false, false, "string_array", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamWithValue<std::vector<std::string>> _string_arrayParam(string_array, _string_arrayDescriptor, dm, false);
Point point{.x{5},.y{10}};
catena::common::ParamDescriptor _pointDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Point"}}, "", "", false, false, "point", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _point_xDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, false, "x", "", nullptr, false, false, dm, 0, 0, 0, false, &_pointDescriptor);
catena::common::ParamDescriptor _point_yDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, false, "y", "", nullptr, false, false, dm, 0, 0, 0, false, &_pointDescriptor);
catena::common::ParamWithValue<params::Point> _pointParam(point, _pointDescriptor, dm, false);
Points points{{.x{1},.y{2}},{.x{3},.y{4}}};
catena::common::ParamDescriptor _pointsDescriptor(st2138::ParamType::STRUCT_ARRAY, {}, {{"en", "Points"}}, "", "", false, false, "points", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _points_xDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, false, "x", "", nullptr, false, false, dm, 0, 0, 0, false, &_pointsDescriptor);
catena::common::ParamDescriptor _points_yDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, false, "y", "", nullptr, false, false, dm, 0, 0, 0, false, &_pointsDescriptor);
catena::common::ParamWithValue<params::Points> _pointsParam(points, _pointsDescriptor, dm, false);
Number number{std::string{"one"}};
catena::common::ParamDescriptor _numberDescriptor(st2138::ParamType::STRUCT_VARIANT, {}, {{"en", "Number"}}, "", "", false, false, "number", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _number_wordsDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "words", "", nullptr, false, false, dm, 0, 0, 0, false, &_numberDescriptor);
catena::common::ParamDescriptor _number_digitsDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, false, "digits", "", nullptr, false, false, dm, 0, 0, 0, false, &_numberDescriptor);
catena::common::ParamWithValue<params::Number> _numberParam(number, _numberDescriptor, dm, false);
Numbers numbers{{std::string{"two"}},{float{3.5}}};
catena::common::ParamDescriptor _numbersDescriptor(st2138::ParamType::STRUCT_VARIANT_ARRAY, {}, {{"en", "Numbers"}}, "", "", false, false, "numbers", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _numbers_labelDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, &_numbersDescriptor);
catena::common::ParamDescriptor _numbers_amountDescriptor(st2138::ParamType::FLOAT32, {}, {}, "", "", false, false, "amount", "", nullptr, false, false, dm, 0, 0, 2, false, &_numbersDescriptor);
catena::common::ParamWithValue<params::Numbers> _numbersParam(numbers, _numbersDescriptor, dm, false);
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
constexpr const char* real_sdk_version = TO_STRING(CATENA_CPP_VERSION);
params::Product& initialize_sdk_version(params::Product& p) {
  p.catena_sdk_version = real_sdk_version;
  return p;
}
params::Product dummy = initialize_sdk_version(product);
