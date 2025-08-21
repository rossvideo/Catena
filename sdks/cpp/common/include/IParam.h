#pragma once

/*
 * Copyright 2024 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
#include <IParamDescriptor.h>
#include <Path.h>
#include <Status.h>

// protobuf interface
#include <interface/param.pb.h>

namespace catena {
namespace common { 

  class IAuthorizer; // Forward declaration

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
     * @brief ParamType is an enum class that defines the types of parameters.
     */
    using ParamType = catena::patterns::EnumDecorator<catena::ParamType>;

    /**
     * @brief OidAliases is a vector of strings.
     */
    using OidAliases = std::vector<std::string>;

    using Path = catena::common::Path;

    IParam() = default;
    virtual ~IParam() = default;

    /**
     * @brief IParam has move semantics.
     */
    IParam& operator=(IParam&&) = default;
    IParam(IParam&&) = default;
    
    /**
     * @brief IParam does not have copy semantics.
     */
    IParam(const IParam&) = delete;
    IParam& operator=(const IParam&) = delete;

    // Virtual clone method
    virtual std::unique_ptr<IParam> copy() const = 0;

    /**
     * @brief Serializes the parameter value to protobuf.
     * @param dst The protobuf value to serialize to.
     * @param authz The authorizer object to containing the client's scopes.
     */
    virtual catena::exception_with_status toProto(catena::Value& dst, const IAuthorizer& authz) const = 0;
    
    /**
     * @brief Deserializes the parameter value from protobuf.
     * @param src The protobuf value to deserialize from.
     * @param authz The authorizer object to containing the client's scopes.
     * @note This method may constrain the source value and modify it.
     */
    virtual catena::exception_with_status fromProto(const catena::Value& src, const IAuthorizer& authz) = 0;

    /**
     * @brief Serialize the parameter descriptor to a protobuf param object.
     * @param param The protobuf param to serialize to.
     * @param authz The authorizer object to containing the client's scopes.
     */
    virtual catena::exception_with_status toProto(catena::Param& param, const IAuthorizer& authz) const = 0;

    /**
     * @brief Serialize the parameter descriptor to a protobuf paramInfo object.
     * @param paramInfo The protobuf paramInfo object to serialize to.
     * @param authz The authorizer object to containing the client's scopes.
     */
    virtual catena::exception_with_status toProto(catena::ParamInfoResponse& paramInfo, const IAuthorizer& authz) const = 0;

    /**
     * @brief Returns the param's protobuf value type.
     */
    virtual ParamType type() const = 0;

    /**
     * @brief Gets the oid of the param
     * @return The oid of the param
     */
    virtual const std::string& getOid() const = 0;

    /**
     * @brief Sets the param's oid.
     * @param oid The new oid for the param.
     */
    virtual void setOid(const std::string& oid) = 0;

    /**
     * @brief Returns true if the param is readOnly.
     */
    virtual bool readOnly() const = 0;

    /**
     * @brief Sets the read only status of the param.
     * @param flag True if the param should be read only, false otherwise.
     */
    virtual void readOnly(bool flag) = 0;

    /**
     * @brief Gets a child parameter by name.
     * @param oid The oid of the child parameter to get.
     * @param authz The IAuthorizer to test read permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return A unique pointer to the child parameter, or nullptr if the
     * operation failed.
     */
    virtual std::unique_ptr<IParam> getParam(Path& oid, const IAuthorizer& authz, catena::exception_with_status& status) = 0;

    /**
     * @brief Returns the size of an array parameter.
     * @return The size of the array parameter, or 0 if the parameter is not an
     * array.
     */
    virtual std::size_t size() const = 0;

    /**
     * @brief Add an empty value to and return the back of an array parameter.
     * @param authz The IAuthorizer to test write permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return A unique ptr to the new element, or nullptr if the operation
     * failed.
     */
    virtual std::unique_ptr<IParam> addBack(const IAuthorizer& authz, catena::exception_with_status& status) = 0;

    /**
     * @brief Adds a child parameter.
     * @param oid The oid of the child parameter to add.
     * @param param The child parameter to add.
     */
    virtual void addParam(const std::string& oid, IParamDescriptor* param) = 0;

    /**
     * @brief Pops the back of an array parameter.
     * @param authz The IAuthorizer to test write permissions with.
     * @return OK if succcessful, otherwise an error.
     */
    virtual catena::exception_with_status popBack(const IAuthorizer& authz) = 0;

    /**
     * @brief Gets the parameter's constraint.
     */
    virtual const IConstraint* getConstraint() const = 0;

    /**
     * @brief Gets the parameter's access scope.
     */
    virtual const std::string& getScope() const = 0;

    /**
     * @brief Defines the parameter's command implementation.
     * @param commandImpl The new command implementation.
     */
    virtual void defineCommand(std::function<std::unique_ptr<IParamDescriptor::ICommandResponder>(const catena::Value&, const bool)> commandImpl) = 0;

    /**
     * @brief Exectutes the parameter's command implementation.
     * @param value the value to pass to the command implementation.
     * @param respond Flag indicating whether the command should respond with
     * a CommandResponse.
     * @return The CommandResponder from the command implementation.
     */
    virtual std::unique_ptr<IParamDescriptor::ICommandResponder> executeCommand(const catena::Value&  value, const bool respond) const = 0;

    /**
     * @brief Gets the parameter's descriptor.
     * @return The parameter's ParamDescriptor object.
     */
    virtual const IParamDescriptor& getDescriptor() const = 0;

    /**
     * @brief Checks if the parameter is an array type.
     * @return True if the parameter is an array type (INT32_ARRAY,
     * FLOAT32_ARRAY, STRING_ARRAY, STRUCT_ARRAY, or STRUCT_VARIANT_ARRAY).
     */
    virtual bool isArrayType() const = 0;
    /**
     * @brief Validates a setValue operation without changing the param's value.
     * @param value The value we want to set the param to.
     * @param index The index of the subparam to set (or nullptr if none).
     * @param authz The authorizer object to check write permissions with.
     * @param ans Catena::exception_with_status output.
     * @returns true if valid.
     */
    virtual bool validateSetValue(const catena::Value& value, Path::Index index, const IAuthorizer& authz, catena::exception_with_status& ans) = 0;
    /**
     * @brief Resets any trackers that might have been changed in
     * validateSetValue().
     */
    virtual void resetValidate() = 0;
};
}  // namespace common

/**
 * @brief A map of parameter types to their string representations.
 */
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
