#ifndef ST2138_SHARED_GEO_H
#define ST2138_SHARED_GEO_H
// This file was auto-generated. Do not modify by hand.
#include <StructInfo.h>
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
#endif // ST2138_SHARED_GEO_H
