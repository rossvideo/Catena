#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace use_variants {
using Number = std::variant<int32_t, std::string>;
struct Cartesian {
    float x;
    float y;
    float z;
};
struct Cylindral {
    float rho;
    float phi;
    float z;
};
struct Spherical {
    float r;
    float theta;
    float phi;
};
using Coordinates = std::vector<std::variant<Cartesian, Cylindral, Spherical>>;
} // namespace use_variants

