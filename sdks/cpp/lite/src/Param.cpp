#include <lite/param.pb.h>

#include <lite/include/Param.h>
#include <lite/include/StructInfo.h>

#include <vector>
#include <string>
#include <type_traits>


using catena::Value;
using catena::lite::Param;
using catena::lite::StructInfo;
using catena::lite::FieldInfo;

template <>
void Param<int32_t>::toProto(Value& dst) const {
    catena::lite::toProto<int32_t>(dst, &value_.get());
}

template <>
void Param<int32_t>::fromProto(Value& src) {
    if (constraint_) {
        constraint_->apply(&src);
    }
    catena::lite::fromProto<int32_t>(&value_.get(), src);
}

template <>
void Param<std::string>::toProto(Value& dst) const {
    catena::lite::toProto<std::string>(dst, &value_.get());
}

template <>
void Param<std::string>::fromProto(Value& src) {
    if (constraint_) {
        constraint_->apply(&src);
    }
    catena::lite::fromProto<std::string>(&value_.get(), src);
}

template <>
void Param<float>::toProto(Value& dst) const {
    catena::lite::toProto<float>(dst, &value_.get());
}

template <>
void Param<float>::fromProto(Value& src) {
    if (constraint_) {
        constraint_->apply(&src);
    }
    catena::lite::fromProto<float>(&value_.get(), src);
}

template <>
void Param<std::vector<std::string>>::toProto(Value& dst) const {
    catena::lite::toProto<std::vector<std::string>>(dst, &value_.get());
}

template <>
void Param<std::vector<std::string>>::fromProto(Value& src) {
    catena::lite::fromProto<std::vector<std::string>>(&value_.get(), src);
}

template <>
void Param<std::vector<std::int32_t>>::toProto(Value& dst) const {
    catena::lite::toProto<std::vector<std::int32_t>>(dst, &value_.get());
}

template <>
void Param<std::vector<std::int32_t>>::fromProto(Value& src) {
    catena::lite::fromProto<std::vector<std::int32_t>>(&value_.get(), src);
}

template <>
void Param<std::vector<float>>::toProto(Value& dst) const {
    catena::lite::toProto<std::vector<float>>(dst, &value_.get());
}

template <>
void Param<std::vector<float>>::fromProto(Value& src) {
    catena::lite::fromProto<std::vector<float>>(&value_.get(), src);
}



