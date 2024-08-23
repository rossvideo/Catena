#pragma once

/**
 * @file IParam.h
 * @brief Interface for parameters
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @date 2024-07-07
 */

#include <lite/param.pb.h>

#include <common/include/Enums.h>
#include <common/include/IConstraint.h>

namespace catena {
class Value; // forward reference
class Param; // forward reference

/// @todo move to common
namespace common { 
class IParam {
  public:
    /**
     * @brief ParamType is an enum class that defines the types of parameters
     */
    using ParamType = catena::patterns::EnumDecorator<catena::ParamType>;

    /**
     * @brief OidAliases is a vector of strings
     */
    using OidAliases = std::vector<std::string>;

  public:
    IParam() = default;
    virtual ~IParam() = default;

    /**
     * @brief IParam has move semantics
     */
    IParam& operator=(IParam&&) = default;
    IParam(IParam&&) = default;
    
    /**
     * @brief IParam does not have copy semantics
     */
    IParam(const IParam&) = delete;
    IParam& operator=(const IParam&) = delete;

    /**
     * @brief serialize the parameter value to protobuf
     * @param dst the protobuf value to serialize to
     */
    virtual void toProto(catena::Value& dst) const = 0;
    
    /**
     * @brief deserialize the parameter value from protobuf
     * @param src the protobuf value to deserialize from
     * @note this method may constrain the source value and modify it
     */
    virtual void fromProto(catena::Value& src) = 0;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param param the protobuf value to serialize to
     */
    virtual void toProto(catena::Param& param) const = 0;

    /**
     * @brief return the type of the param
     */
    virtual ParamType type() const = 0;

    /**
     * @brief return the oid of the param
     * @return the oid of the param
     */
    virtual const std::string& getOid() const = 0;

    /**
     * @brief set the oid of the param
     */
    virtual void setOid(const std::string& oid) = 0;

    /**
     * @brief return read only status of the param
     */
    virtual bool readOnly() const = 0;

    /**
     * @brief set read only status of the param
     */
    virtual void readOnly(bool flag) = 0;

    /**
     * @brief get a child parameter by name
     */
    virtual IParam* getParam(const std::string& name) = 0;

    /**
     * @brief add a child parameter
     */
    virtual void addParam(const std::string& oid, IParam* param) = 0;

    /**
     * @brief get a constraint by oid
     */
    virtual IConstraint* getConstraint(const std::string& oid) = 0;

    /**
     * @brief add a constraint
     */
    virtual void addConstraint(const std::string& oid, IConstraint* constraint) = 0;
};
}  // namespace common

template<>
const inline common::IParam::ParamType::FwdMap 
  common::IParam::ParamType::fwdMap_ {
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

 