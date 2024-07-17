#include <lite/include/StructInfo.h>

#include <lite/param.pb.h>


template<>
void catena::lite::toProto<float>(catena::Value& value, const void* src) {
    value.set_float32_value(*reinterpret_cast<const float*>(src));
}

template<>
void catena::lite::toProto<int32_t>(Value& value, const void* src) {
    value.set_int32_value(*reinterpret_cast<const int32_t*>(src));
}

template<>
void catena::lite::toProto<std::string>(Value& value, const void* src) {
    *value.mutable_string_value() = *reinterpret_cast<const std::string*>(src);
}

template<>
void catena::lite::toProto<std::vector<std::string>>(Value& value, const void* src) {
    value.clear_string_array_values();
    auto& string_array = *value.mutable_string_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<std::string>*>(src);
    for (const auto& s : vec) {
        string_array.add_strings(s);
    }
}

template<>
void catena::lite::toProto<std::vector<int32_t>>(Value& value, const void* src) {
    value.clear_int32_array_values();
    auto& int_array = *value.mutable_int32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<int32_t>*>(src);
    for (const auto& i : vec) {
        int_array.add_ints(i);
    }
}

template<>
void catena::lite::toProto<std::vector<float>>(Value& value, const void* src) {
    value.clear_float32_array_values();
    auto& float_array = *value.mutable_float32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<float>*>(src);
    for (const auto& f : vec) {
        float_array.add_floats(f);
    }
}
