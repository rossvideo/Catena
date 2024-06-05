#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Meta.h>
#include <TypeTraits.h>
#include <string>
#include <vector>
namespace one_of_everything {
struct Outer_nested_struct {
  struct Inner_nested_struct {
    int32_t num_1;
    int32_t num_2;
    static const StructInfo& getStructInfo();
  };
  Inner_nested_struct inner_nested_struct;
  std::string text;
  static const StructInfo& getStructInfo();
};
} // namespace one_of_everything
