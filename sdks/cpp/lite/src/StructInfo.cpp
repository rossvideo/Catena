#include <lite/include/StructInfo.h>

#include <interface/param.pb.h>


template<>
void catena::lite::toProto<float>(catena::Value& dst, const void* src) {
    dst.set_float32_value(*reinterpret_cast<const float*>(src));
}

template<>
void catena::lite::fromProto<float>(void* dst, const catena::Value& src) {
    *reinterpret_cast<float*>(dst) = src.float32_value();
}

template<>
void catena::lite::toProto<int32_t>(Value& dst, const void* src) {
    dst.set_int32_value(*reinterpret_cast<const int32_t*>(src));
}

template<>
void catena::lite::fromProto<int32_t>(void* dst, const catena::Value& src) {
    *reinterpret_cast<int32_t*>(dst) = src.int32_value();
}

template<>
void catena::lite::toProto<std::string>(Value& dst, const void* src) {
    *dst.mutable_string_value() = *reinterpret_cast<const std::string*>(src);
}

template<>
void catena::lite::fromProto<std::string>(void* dst, const catena::Value& src) {
    *reinterpret_cast<std::string*>(dst) = src.string_value();
}

template<>
void catena::lite::toProto<std::vector<std::string>>(Value& dst, const void* src) {
    dst.clear_string_array_values();
    auto& string_array = *dst.mutable_string_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<std::string>*>(src);
    for (const auto& s : vec) {
        string_array.add_strings(s);
    }
}

template<>
void catena::lite::fromProto<std::vector<std::string>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<std::string>*>(dst);
    vec->clear();
    const auto& string_array = src.string_array_values();
    for (int i = 0; i < string_array.strings_size(); ++i) {
        vec->push_back(string_array.strings(i));
    }
}

template<>
void catena::lite::toProto<std::vector<int32_t>>(Value& dst, const void* src) {
    dst.clear_int32_array_values();
    auto& int_array = *dst.mutable_int32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<int32_t>*>(src);
    for (const auto& i : vec) {
        int_array.add_ints(i);
    }
}

template<>
void catena::lite::fromProto<std::vector<int32_t>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<int32_t>*>(dst);
    vec->clear();
    const auto& int_array = src.int32_array_values();
    for (int i = 0; i < int_array.ints_size(); ++i) {
        vec->push_back(int_array.ints(i));
    }
}

template<>
void catena::lite::toProto<std::vector<float>>(Value& dst, const void* src) {
    dst.clear_float32_array_values();
    auto& float_array = *dst.mutable_float32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<float>*>(src);
    for (const auto& f : vec) {
        float_array.add_floats(f);
    }
}

template<>
void catena::lite::fromProto<std::vector<float>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<float>*>(dst);
    vec->clear();
    const auto& float_array = src.float32_array_values();
    for (int i = 0; i < float_array.floats_size(); ++i) {
        vec->push_back(float_array.floats(i));
    }
}
