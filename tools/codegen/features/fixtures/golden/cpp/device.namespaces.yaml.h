#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace shared::geo {
  struct Point {
    float latitude;
    float longitude;
    using isCatenaStruct = void;
  };
  struct Segment {
    shared::geo::Point start;
    shared::geo::Point end;
    using isCatenaStruct = void;
  };
} // namespace shared::geo
namespace namespaces {
struct Product {
  std::string name;
  std::string vendor;
  std::string version;
  std::string catena_sdk;
  std::string catena_sdk_version;
  std::string serial_number;
  using isCatenaStruct = void;
};
} // namespace namespaces
template<>
struct catena::common::StructInfo<shared::geo::Point> {
  using Point = shared::geo::Point;
  using Type = std::tuple<FieldInfo<float, Point>, FieldInfo<float, Point>>;
  static constexpr Type fields = {{"latitude", &Point::latitude}, {"longitude", &Point::longitude}};
};
template<>
struct catena::common::StructInfo<shared::geo::Segment> {
  using Segment = shared::geo::Segment;
  using Type = std::tuple<FieldInfo<shared::geo::Point, Segment>, FieldInfo<shared::geo::Point, Segment>>;
  static constexpr Type fields = {{"start", &Segment::start}, {"end", &Segment::end}};
};
template<>
struct catena::common::StructInfo<namespaces::Product> {
  using Product = namespaces::Product;
  using Type = std::tuple<FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>, FieldInfo<std::string, Product>>;
  static constexpr Type fields = {{"name", &Product::name}, {"vendor", &Product::vendor}, {"version", &Product::version}, {"catena_sdk", &Product::catena_sdk}, {"catena_sdk_version", &Product::catena_sdk_version}, {"serial_number", &Product::serial_number}};
};
