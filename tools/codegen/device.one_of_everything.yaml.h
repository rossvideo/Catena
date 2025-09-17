#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::common::Device dm;
namespace one_of_everything {
struct Ref_struct {
  struct Nested_struct {
    int32_t num_1;
    int32_t num_2;
    using isCatenaStruct = void;
  };
  Nested_struct nested_struct;
  using isCatenaStruct = void;
};
using Struct_array = std::vector<one_of_everything::Ref_struct>;
struct Constraint_examples {
  int32_t int32_range{6};
  int32_t int32_choice{1};
  float float32_range{10.5};
  std::string string_choice{"a"};
  std::string string_string_choice{"<#FF0000>"};
  std::string string_length{"Hello worl"};
  std::vector<int32_t> int_array_range{0, 2, 4, 6, 8, 10};
  std::vector<int32_t> int_array_choice{0, 1, 0, 0, 1};
  std::vector<float> float_array_range;
  std::vector<std::string> string_array_choice{"a", "b", "a"};
  std::vector<std::string> string_string_array_choice{"<#FF0000>", "<#00FF00>", "<#0000FF>"};
  std::vector<std::string> string_array_length{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
  using isCatenaStruct = void;
};
} // namespace one_of_everything
template<>
struct catena::common::StructInfo<one_of_everything::Ref_struct::Nested_struct> {
  using Nested_struct = one_of_everything::Ref_struct::Nested_struct;
  using Type = std::tuple<FieldInfo<int32_t, Nested_struct>, FieldInfo<int32_t, Nested_struct>>;
  static constexpr Type fields = {{"num_1", &Nested_struct::num_1}, {"num_2", &Nested_struct::num_2}};
};
template<>
struct catena::common::StructInfo<one_of_everything::Ref_struct> {
  using Ref_struct = one_of_everything::Ref_struct;
  using Type = std::tuple<FieldInfo<one_of_everything::Ref_struct::Nested_struct, Ref_struct>>;
  static constexpr Type fields = {{"nested_struct", &Ref_struct::nested_struct}};
};
template<>
struct catena::common::StructInfo<one_of_everything::Constraint_examples> {
  using Constraint_examples = one_of_everything::Constraint_examples;
  using Type = std::tuple<FieldInfo<int32_t, Constraint_examples>, FieldInfo<int32_t, Constraint_examples>, FieldInfo<float, Constraint_examples>, FieldInfo<std::string, Constraint_examples>, FieldInfo<std::string, Constraint_examples>, FieldInfo<std::string, Constraint_examples>, FieldInfo<std::vector<int32_t>, Constraint_examples>, FieldInfo<std::vector<int32_t>, Constraint_examples>, FieldInfo<std::vector<float>, Constraint_examples>, FieldInfo<std::vector<std::string>, Constraint_examples>, FieldInfo<std::vector<std::string>, Constraint_examples>, FieldInfo<std::vector<std::string>, Constraint_examples>>;
  static constexpr Type fields = {{"int32_range", &Constraint_examples::int32_range}, {"int32_choice", &Constraint_examples::int32_choice}, {"float32_range", &Constraint_examples::float32_range}, {"string_choice", &Constraint_examples::string_choice}, {"string_string_choice", &Constraint_examples::string_string_choice}, {"string_length", &Constraint_examples::string_length}, {"int_array_range", &Constraint_examples::int_array_range}, {"int_array_choice", &Constraint_examples::int_array_choice}, {"float_array_range", &Constraint_examples::float_array_range}, {"string_array_choice", &Constraint_examples::string_array_choice}, {"string_string_array_choice", &Constraint_examples::string_string_array_choice}, {"string_array_length", &Constraint_examples::string_array_length}};
};
