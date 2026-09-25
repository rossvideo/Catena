#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace commands {
struct Product {
  std::string name;
  std::string vendor;
  std::string version;
  std::string st2138_sdk;
  std::string st2138_sdk_version;
  std::string serial_number;
  using isCatenaStruct = void;
};
} // namespace commands
template<>
struct catena::common::StructInfo<commands::Product> {
  using Product = commands::Product;
  using Type = std::tuple<FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>>;
  static constexpr Type fields = {{"name", &Product::name}, {"vendor", &Product::vendor}, {"version", &Product::version}, {"st2138_sdk", &Product::st2138_sdk}, {"st2138_sdk_version", &Product::st2138_sdk_version}, {"serial_number", &Product::serial_number}};
};
