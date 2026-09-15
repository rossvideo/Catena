#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace menus {
struct Product {
  std::string name;
  std::string vendor;
  std::string version;
  std::string catena_sdk;
  std::string catena_sdk_version;
  std::string serial_number;
  using isCatenaStruct = void;
};
} // namespace menus
template<>
struct catena::common::StructInfo<menus::Product> {
  using Product = menus::Product;
  using Type = std::tuple<FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>>;
  static constexpr Type fields = {{"name", &Product::name}, {"vendor", &Product::vendor}, {"version", &Product::version}, {"catena_sdk", &Product::catena_sdk}, {"catena_sdk_version", &Product::catena_sdk_version}, {"serial_number", &Product::serial_number}};
};
