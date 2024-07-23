#pragma once

/// @todo some cmake + preprocessing magic to include the right file - lite or full version
#include <lite/param.pb.h>

#include <Enums.h>

namespace catena {
class Value; // forward reference
class Param; // forward reference


namespace lite {
class IParam {
  public:
    using ParamType = catena::patterns::EnumDecorator<catena::ParamType>;
  public:
    IParam() = default;
    IParam(std::string oid) : oid_(oid) {};
    IParam(IParam&&) = default;
    IParam& operator=(IParam&&) = default;
    virtual ~IParam() = default;

    /**
     * @brief serialize the parameter value to protobuf
     * @param value the protobuf value to serialize to
     */
    virtual void toProto(catena::Value& value) const = 0;
    
    /**
     * @brief deserialize the parameter value from protobufi
     * @param value the protobuf value to deserialize from
     */
    virtual void fromProto(const catena::Value& value) = 0;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param param the protobuf value to serialize to
     */
    virtual void toProto(catena::Param& param) const = 0;

    virtual ParamType type() const = 0;

    /**
     * @brief return the oid of the param
     */
    virtual const std::string& getOid() const { return oid_; };

    virtual void setOid(const std::string& oid) { oid_ = oid; };

   protected:
    std::string oid_;
};
}  // namespace lite

template<>
const inline lite::IParam::ParamType::FwdMap 
  lite::IParam::ParamType::fwdMap_ {
  { ParamType::UNDEFINED, "undefined" },
  { ParamType::EMPTY, "empty"},
  { ParamType::INT32, "int32" },
  { ParamType::FLOAT32, "float32" },
  { ParamType::STRING, "string" },
  { ParamType::STRUCT, "struct" },
  { ParamType::STRUCT_VARIANT, "struct_variant" },
  { ParamType::INT32_ARRAY, "int32_array" },
  { ParamType::FLOAT32_ARRAY, "float32_array" },
  { ParamType::STRING_ARRAY, "string_array" },
  { ParamType::BINARY, "binary" },
  { ParamType::STRUCT_ARRAY, "struct_array" },
  { ParamType::STRUCT_VARIANT_ARRAY, "struct_variant_array" },
  { ParamType::DATA, "data" }
};
}  // namespace catena

 