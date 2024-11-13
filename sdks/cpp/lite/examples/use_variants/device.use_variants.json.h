#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace use_variants {
using Number = std::variant<int32_t, std::string>;
struct Cartesian {
    int32_t x;
    int32_t y;
    int32_t z;
    using isCatenaStruct = void;
};
namespace _coordinates {
struct Cylindrical {
    int32_t rho;
    int32_t phi;
    int32_t z;
    using isCatenaStruct = void;
};
struct Spherical {
    int32_t r;
    int32_t theta;
    int32_t phi;
    using isCatenaStruct = void;
};
} // namespace _coordinates
using Coordinates_elem = std::variant<use_variants::Cartesian, _coordinates::Cylindrical, _coordinates::Spherical>;
using Coordinates = std::vector<Coordinates_elem>;
} // namespace use_variants
template<>
const std::vector<std::string> catena::lite::AlternativeNames<use_variants::Number>{"digits", "words"};
template<>
struct catena::lite::StructInfo<use_variants::Cartesian> {
    using Cartesian = use_variants::Cartesian;
    using Type = std::tuple<FieldInfo<int32_t, Cartesian>, FieldInfo<int32_t, Cartesian>, FieldInfo<int32_t, Cartesian>>;
    static constexpr Type fields = {{"x", &Cartesian::x}, {"y", &Cartesian::y}, {"z", &Cartesian::z}};
};
template<>
struct catena::lite::StructInfo<use_variants::_coordinates::Cylindrical> {
    using Cylindrical = use_variants::_coordinates::Cylindrical;
    using Type = std::tuple<FieldInfo<int32_t, Cylindrical>, FieldInfo<int32_t, Cylindrical>, FieldInfo<int32_t, Cylindrical>>;
    static constexpr Type fields = {{"rho", &Cylindrical::rho}, {"phi", &Cylindrical::phi}, {"z", &Cylindrical::z}};
};
template<>
struct catena::lite::StructInfo<use_variants::_coordinates::Spherical> {
    using Spherical = use_variants::_coordinates::Spherical;
    using Type = std::tuple<FieldInfo<int32_t, Spherical>, FieldInfo<int32_t, Spherical>, FieldInfo<int32_t, Spherical>>;
    static constexpr Type fields = {{"r", &Spherical::r}, {"theta", &Spherical::theta}, {"phi", &Spherical::phi}};
};
template<>
const std::vector<std::string> catena::lite::AlternativeNames<use_variants::Coordinates_elem>{"cartesian", "cylindrical", "spherical"};


