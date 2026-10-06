#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace params {
struct Product {
  std::string name;
  std::string vendor;
  std::string version;
  std::string st2138_sdk;
  std::string st2138_sdk_version;
  std::string serial_number;
  using isCatenaStruct = void;
};
struct Point {
  int32_t x;
  int32_t y;
  using isCatenaStruct = void;
};
struct Points_elem {
  int32_t x;
  int32_t y;
  using isCatenaStruct = void;
};
using Points = std::vector<Points_elem>;
namespace _number {
} // namespace _number
using Number = std::variant<std::string, int32_t>;
namespace _numbers {
  struct Rational {
    int32_t numerator;
    int32_t denominator;
    using isCatenaStruct = void;
  };
} // namespace _numbers
using Numbers_elem = std::variant<std::string, float, params::_numbers::Rational>;
using Numbers = std::vector<Numbers_elem>;
} // namespace params
template<>
struct catena::common::StructInfo<params::Product> {
  using Product = params::Product;
  using Type = std::tuple<FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>>;
  static constexpr Type fields = {{"name", &Product::name}, {"vendor", &Product::vendor}, {"version", &Product::version}, {"st2138_sdk", &Product::st2138_sdk}, {"st2138_sdk_version", &Product::st2138_sdk_version}, {"serial_number", &Product::serial_number}};
};
template<>
struct catena::common::StructInfo<params::Point> {
  using Point = params::Point;
  using Type = std::tuple<FieldInfo<int32_t, Point>, FieldInfo<int32_t, Point>>;
  static constexpr Type fields = {{"x", &Point::x}, {"y", &Point::y}};
};
template<>
struct catena::common::StructInfo<params::Points_elem> {
  using Points = params::Points_elem;
  using Type = std::tuple<FieldInfo<int32_t, Points>, FieldInfo<int32_t, Points>>;
  static constexpr Type fields = {{"x", &Points::x}, {"y", &Points::y}};
};
template<>
inline std::array<const char*, 2> catena::common::alternativeNames<params::Number>{"words", "digits"};
template<>
struct catena::common::StructInfo<params::_numbers::Rational> {
  using Rational = params::_numbers::Rational;
  using Type = std::tuple<FieldInfo<int32_t, Rational>, FieldInfo<int32_t, Rational>>;
  static constexpr Type fields = {{"numerator", &Rational::numerator}, {"denominator", &Rational::denominator}};
};
template<>
inline std::array<const char*, 3> catena::common::alternativeNames<params::Numbers_elem>{"label", "amount", "rational"};
