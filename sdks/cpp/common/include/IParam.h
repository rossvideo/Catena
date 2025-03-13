#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

/**
 * @file IParam.h
 * @brief Interface for parameters
 * @author John R. Naylor, john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

//common 
#include <Enums.h>
#include <IConstraint.h>
#include <Path.h>
#include <Status.h>

// protobuf interface
#include <interface/param.pb.h>

namespace catena {
namespace common { 

  class Authorizer;
  class ParamDescriptor;  

/**
 * @brief IParam is the interface for business logic and connection logic to interact with parameters
 * 
 * This class creates an interface for accessing catena parameter without needing to know any 
 * type information about the parameter. This allows the connection logic to be decoupled from the
 * generated device code.
 */
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

    using Path = catena::common::Path;

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

    // Virtual clone method
    virtual std::unique_ptr<IParam> copy() const = 0;

    /**
     * @brief serialize the parameter value to protobuf
     * @param dst the protobuf value to serialize to
     */
    virtual catena::exception_with_status toProto(catena::Value& dst, Authorizer& authz) const = 0;
    
    /**
     * @brief deserialize the parameter value from protobuf
     * @param src the protobuf value to deserialize from
     * @note this method may constrain the source value and modify it
     */
    virtual catena::exception_with_status fromProto(const catena::Value& src, Authorizer& authz) = 0;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param param the protobuf value to serialize to
     */
    virtual catena::exception_with_status toProto(catena::Param& param, Authorizer& authz) const = 0;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param paramInfo the protobuf value to serialize to
     */
    virtual catena::exception_with_status toProto(catena::BasicParamInfoResponse& paramInfo, Authorizer& authz) const = 0;

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
     * @brief Validates the size of a string or array value.
     */
    virtual bool validateSize(const catena::Value& value) const = 0;

    /**
     * @brief get a child parameter by name
     */
    virtual std::unique_ptr<IParam> getParam(Path& oid, Authorizer& authz, catena::exception_with_status& status) = 0;

    /**
     * @brief Return the size of an array parameter.
     * @return The size of the array parameter, or 0 if the parameter is not an
     * array.
     */
    virtual uint32_t size() const = 0;

    /**
     * @brief Add an empty value to and return the back of an array parameter.
     * @param authz The Authorizer to test write permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return A unique ptr to the new element, or nullptr if the operation
     * failed.
     */
    virtual std::unique_ptr<IParam> addBack(Authorizer& authz, catena::exception_with_status& status) = 0;

    /**
     * @brief Pop the back of an array parameter.
     * @param authz The Authorizer to test write permissions with.
     * @return OK if succcessful, otherwise an error.
     */
    virtual catena::exception_with_status popBack(Authorizer& authz) = 0;

    /**
     * @brief get a constraint by oid
     */
    virtual const IConstraint* getConstraint() const = 0;

    /**
     * @brief get the parameter access scope
     */
    virtual const std::string& getScope() const = 0;

    /**
     * @brief define a command for the parameter
     */
    virtual void defineCommand(std::function<catena::CommandResponse(catena::Value)> command) = 0;

    /**
     * @brief execute the command for the parameter
     */
    virtual catena::CommandResponse executeCommand(const catena::Value&  value) const = 0;

    /**
     * @brief get the descriptor of the parameter
     * @return the descriptor of the parameter
     */
    virtual const ParamDescriptor& getDescriptor() const = 0;

    /**
     * @brief Check if the parameter is an array type
     * @return true if the parameter is an array type (INT32_ARRAY, FLOAT32_ARRAY, STRING_ARRAY, STRUCT_ARRAY, or STRUCT_VARIANT_ARRAY)
     */
    virtual bool isArrayType() const = 0;

    virtual catena::exception_with_status validateSetValue(const catena::Value& value, uint32_t index) = 0;
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

 