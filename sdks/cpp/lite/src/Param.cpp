#include <lite/param.pb.h>

#include <lite/include/Param.h>
#include <lite/include/StructInfo.h>

#include <vector>
#include <string>
#include <type_traits>


using catena::Value;
using catena::lite::Param;
using catena::meta::has_getStructInfo;
using catena::lite::StructInfo;
using catena::lite::FieldInfo;


template <>
void Param<int32_t>::toProto(Value& value) const {
    catena::lite::toProto<int32_t>(value, &value_.get());
}

template <>
void Param<int32_t>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<int32_t>(&value_.get(), value);
}

template <>
void Param<std::string>::toProto(Value& value) const {
    catena::lite::toProto<std::string>(value, &value_.get());
}

template <>
void Param<std::string>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<std::string>(&value_.get(), value);
}

template <>
void Param<float>::toProto(Value& value) const {
    catena::lite::toProto<float>(value, &value_.get());
}

template <>
void Param<float>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<float>(&value_.get(), value);
}

template <>
void Param<std::vector<std::string>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<std::string>>(value, &value_.get());
}

template <>
void Param<std::vector<std::string>>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<std::vector<std::string>>(&value_.get(), value);
}

template <>
void Param<std::vector<std::int32_t>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<std::int32_t>>(value, &value_.get());
}

template <>
void Param<std::vector<std::int32_t>>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<std::vector<std::int32_t>>(&value_.get(), value);
}

template <>
void Param<std::vector<float>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<float>>(value, &value_.get());
}

template <>
void Param<std::vector<float>>::fromProto(const Value& value) {
    if (read_only_) throw std::runtime_error("Cannot set read-only parameter");
    catena::lite::fromProto<std::vector<float>>(&value_.get(), value);
}



