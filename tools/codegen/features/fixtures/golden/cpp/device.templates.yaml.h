#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace templates {
struct Product {
  std::string name;
  std::string vendor;
  std::string version;
  std::string st2138_sdk;
  std::string st2138_sdk_version;
  std::string serial_number;
  using isCatenaStruct = void;
};
struct Channel_template {
  std::string label;
  float gain;
  int32_t mute;
  struct Eq {
    float low;
    float mid;
    float high;
    using isCatenaStruct = void;
  };
  Eq eq;
  using isCatenaStruct = void;
};
using Aux_sends = std::vector<templates::Channel_template>;
} // namespace templates
template<>
struct catena::common::StructInfo<templates::Product> {
  using Product = templates::Product;
  using Type = std::tuple<FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>>;
  static constexpr Type fields = {{"name", &Product::name}, {"vendor", &Product::vendor}, {"version", &Product::version}, {"st2138_sdk", &Product::st2138_sdk}, {"st2138_sdk_version", &Product::st2138_sdk_version}, {"serial_number", &Product::serial_number}};
};
template<>
struct catena::common::StructInfo<templates::Channel_template::Eq> {
  using Eq = templates::Channel_template::Eq;
  using Type = std::tuple<FieldInfo<float, Eq>, FieldInfo<float, Eq>, FieldInfo<float, Eq>>;
  static constexpr Type fields = {{"low", &Eq::low}, {"mid", &Eq::mid}, {"high", &Eq::high}};
};
template<>
struct catena::common::StructInfo<templates::Channel_template> {
  using Channel_template = templates::Channel_template;
  using Type = std::tuple<FieldInfo<std::string, Channel_template>, FieldInfo<float, Channel_template>, FieldInfo<int32_t, Channel_template>, FieldInfo<templates::Channel_template::Eq, Channel_template>>;
  static constexpr Type fields = {{"label", &Channel_template::label}, {"gain", &Channel_template::gain}, {"mute", &Channel_template::mute}, {"eq", &Channel_template::eq}};
};
