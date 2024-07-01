#include <lite/include/Param.h>
#include <lite/param.pb.h>
#include <lite/include/StructInfo.h>

#include <vector>
#include <string>
#include <type_traits>

using catena::Value;
using catena::lite::Param;
using catena::meta::has_getStructInfo;
using catena::lite::StructInfo;
using catena::lite::FieldInfo;

// 
template <>
void Param<std::string>::serialize(Value& value) const {
    *value.mutable_string_value() = value_.get();
}

template <>
void Param<std::int32_t>::serialize(Value& value) const {
    value.set_int32_value(value_.get());
}

template <>
void Param<float>::serialize(Value& value) const {
    value.set_float32_value(value_.get());
}

template <>
void Param<std::vector<std::string>>::serialize(Value& value) const {
    value.clear_string_array_values();
    auto& string_array = *value.mutable_string_array_values();
    for (const auto& s : value_.get()) {
        string_array.add_strings(s);
    }
}

template <>
void Param<std::vector<std::int32_t>>::serialize(Value& value) const {
    value.clear_int32_array_values();
    auto& int_array = *value.mutable_int32_array_values();
    for (const auto& i : value_.get()) {
        int_array.add_ints(i);
    }
}

template <>
void Param<std::vector<float>>::serialize(Value& value) const {
    value.clear_float32_array_values();
    auto& float_array = *value.mutable_float32_array_values();
    for (const auto& f : value_.get()) {
        float_array.add_floats(f);
    }
}

