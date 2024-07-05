#include <lite/include/Param.h>

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
void Param<std::string>::toProto(Value& value) const {
    catena::lite::toProto<std::string>(value, &value_.get());
}

template <>
void Param<float>::toProto(Value& value) const {
    catena::lite::toProto<float>(value, &value_.get());
}

template <>
void Param<std::vector<std::string>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<std::string>>(value, &value_.get());
}

template <>
void Param<std::vector<std::int32_t>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<std::int32_t>>(value, &value_.get());
}

template <>
void Param<std::vector<float>>::toProto(Value& value) const {
    catena::lite::toProto<std::vector<float>>(value, &value_.get());
}


