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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @file ParamWithValue.h
 * @brief Interface for acessing parameter values
 * @author John R. Naylor
 * @author John Danen
 * @date 2024-08-20
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IParam.h>
#include <Tags.h>
#include <Path.h>
#include <ParamDescriptor.h>
#include <Device.h>
#include <StructInfo.h>
#include <PolyglotText.h>
#include <AuthzInfo.h>

// protobuf interface
#include <interface/param.pb.h>

#include <functional>
#include <string>
#include <memory>

namespace catena {
namespace common {

/**
 * @brief ParamWithValue is a class that implements the IParam interface
 * for each param type in the device model
 */
template <typename T>
class ParamWithValue : public catena::common::IParam {
  public:
    ParamWithValue() = delete;

    /**
     * @brief Construct a new ParamWithValue object and add it to the device
     */
    ParamWithValue(
        T& value,
        ParamDescriptor& descriptor,
        Device &dev,
        bool isCommand
    ) : value_{value}, descriptor_{descriptor} {
        if (isCommand) {
            dev.addItem<common::CommandTag>(descriptor.getOid(), this);
        } else {
            dev.addItem<common::ParamTag>(descriptor.getOid(), this);
        }
    }

    /**
     * @brief Construct a new ParamWithValue object without adding it to device
     */
    ParamWithValue(
        T& value,
        ParamDescriptor& descriptor
    ) : value_{value}, descriptor_{descriptor} {}

    /**
     * @brief Construct a new ParamWithValue object using FieldInfo
     */
    template <typename FieldType = T, typename ParentType>
    ParamWithValue(
        const FieldInfo<FieldType, ParentType>& field, 
        ParentType& parentValue,
        ParamDescriptor& parentDescriptor
    ) : descriptor_{parentDescriptor.getSubParam(field.name)}, value_{(parentValue.*(field.memberPtr))} {}

    /**
     * @brief ParamWithValue can not be copied directly
     */
    ParamWithValue(const ParamWithValue&) = delete;

    /**
     * @brief ParamWithValue can not be copied directly
     */
    ParamWithValue& operator=(const ParamWithValue&) = delete;

    /**
     * @brief ParamWithValue has move semantics
     */
    ParamWithValue(ParamWithValue&&) = default;

    /**
     * @brief ParamWithValue has move semantics
     */
    ParamWithValue& operator=(ParamWithValue&&) = default;

    /**
     * @brief default destructor
     */
    virtual ~ParamWithValue() = default;

    /**
     * @brief creates a shallow copy the parameter
     * 
     * Needed to copy IParam objects
     * 
     * ParamWithValue objects only contain two references, so they are cheap to copy
     */
    std::unique_ptr<IParam> copy() const override {
        return std::make_unique<ParamWithValue<T>>(value_.get(), descriptor_);
    }

    /**
     * @brief serialize the parameter value to protobuf if authorized
     * @param value the protobuf value to serialize to
     * @param clientScope the client scope
     */
    catena::exception_with_status toProto(catena::Value& value, std::string& clientScope) const override {
        AuthzInfo auth(descriptor_, clientScope);
        if (!auth.readAuthz()) {
            return catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
        toProto<T>(value, &value_.get(), auth);
        return catena::exception_with_status("", catena::StatusCode::OK);
    }

    /**
     * @brief serialize the parameter descriptor to protobuf
     * include both the descriptor and the value
     * @param param the protobuf value to serialize to
     * @param clientScope the client scope
     */
    catena::exception_with_status toProto(catena::Param& param, std::string& clientScope) const override {
        AuthzInfo auth(descriptor_, clientScope);
        if (!auth.readAuthz()) {
            return catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
        descriptor_.toProto(param, auth);        
        toProto<T>(*param.mutable_value(), &value_.get(), auth);
        return catena::exception_with_status("", catena::StatusCode::OK);
    }

    /**
     * @brief deserialize the parameter value from protobuf if authorized
     * @param value the protobuf value to deserialize from
     * @param clientScope the client scope
     */
    catena::exception_with_status fromProto(const catena::Value& value, std::string& clientScope) override {
        AuthzInfo auth(descriptor_, clientScope);
        if (!auth.readAuthz()) {
            return catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
        if (!auth.writeAuthz()) {
            return catena::exception_with_status("Not authorized to write to param", catena::StatusCode::PERMISSION_DENIED);
        } else {
            fromProto<T>(value, &value_.get(), auth);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }
    }

    /**
     * @brief get the parameters protobuf value type
     */
    typename IParam::ParamType type() const override {
        return descriptor_.type();
    }

    /** 
     * @brief get the parameter oid
     */
    const std::string& getOid() const override {
        return descriptor_.getOid();
    }

    /**
     * @brief set the parameter oid
     */
    void setOid(const std::string& oid) override{
        descriptor_.setOid(oid);
    }

    /** 
     * @brief return the params read only flag
     */
    bool readOnly() const override {
        return descriptor_.readOnly();
    }

    /**
     * @brief set the params read only flag
     */
    void readOnly(bool flag) override {
        descriptor_.readOnly(flag);
    }

    /**
     * @brief get the value of the parameter
     */
    T& get() {
        return value_.get();
    }

    /**
     * @brief get the value of the parameter (const version)
     */
    const T& get() const {
        return value_.get();
    }

    /**
     * @brief get a child parameter by name
     * @param oid the path to the child parameter
     * @return a unique pointer to the child parameter, or nullptr if it does not exist
     */
    std::unique_ptr<IParam> getParam(Path& oid, catena::exception_with_status& status) override {
        return getParam_(oid, value_.get(), status);
    }

    /**
     * @brief define a command for the parameter
     */
    void defineCommand(std::function<catena::CommandResponse(catena::Value)> command) override {
        descriptor_.defineCommand(command);
    }

    /**
     * @brief execute the command for the parameter
     */
    catena::CommandResponse executeCommand(const catena::Value& value) const override {
        return descriptor_.executeCommand(value);
    }

  private:

    /**
     * @brief get the child parameter that oid points to
     * @tparam U the type of the value that we are getting the child parameter from
     * @param oid the path to the child parameter
     * @param value the value to get the child parameter from
     * @return a unique pointer to the child parameter
     * 
     * this generic template is used when the type is not a CatenaStruct or CatenaStructArray.
     * Since this type has no sub-params, it only returns nullptr.
     * 
     */
    template <typename U>
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, catena::exception_with_status& status) {
        // This type is not a CatenaStruct or CatenaStructArray so it has no sub-params
        status = catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
        return nullptr;
    }

    /**
     * @brief get the child parameter that oid points to
     * @tparam U the type of the value that we are getting the child parameter from
     * @param oid the path to the child parameter
     * @param value the value to get the child parameter from
     * @return a unique pointer to the child parameter, or nullptr if it does not exist
     * 
     * this specialization is used when the type is a CatenaStructArray.
     * This function expects the front segment of the oid to be an index.
     */
    template<meta::IsVector U>
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, catena::exception_with_status& status) {
        using ElemType = U::value_type;
        if (!oid.front_is_index()) {
            status = catena::exception_with_status("Expected index in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        size_t oidIndex = oid.front_as_index();
        oid.pop();

        if (oidIndex == catena::common::Path::kEnd) {
            // Index is "-", add a new element to the array
            oidIndex = value.size();
            value.push_back(ElemType());
        } else if (oidIndex >= value.size()) {
            // If index is out of bounds, return nullptr
            status = catena::exception_with_status("Index out of bounds in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }

        if (oid.empty()) {
            // we reached the end of the path, return the element
            return std::make_unique<ParamWithValue<ElemType>>(value[oidIndex], descriptor_);
        } else {
            if constexpr (CatenaStruct<ElemType>) {
                // The path has more segments, keep recursing
                return ParamWithValue<ElemType>(value[oidIndex], descriptor_).getParam(oid, status);
            } else {
                // This type is not a CatenaStructArray so it has no sub-params
                status = catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
                return nullptr;
            }
        }
    }

    /**
     * @brief get the child parameter that oid points to
     * @tparam U the type of the value that we are getting the child parameter from
     * @param oid the path to the child parameter
     * @param value the value to get the child parameter from
     * @return a unique pointer to the child parameter, or nullptr if it does not exist
     * 
     * this specialization is used when the type is a CatenaStruct.
     * This function expects the front segment of the oid to be a string.
     */
    template <CatenaStruct U>
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, catena::exception_with_status& status) {
        if (!oid.front_is_string()) {
            status = catena::exception_with_status("Expected string in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        std::string oidStr = oid.front_as_string();
        oid.pop();
        auto fields = StructInfo<U>::fields; // get tuple of FieldInfo for this struct type

        std::unique_ptr<IParam> ip = findParamByName_<U>(fields, oidStr);
        if (!ip) {
            status = catena::exception_with_status("Param does not exist", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }

        if (oid.empty()) {
            // we reached the end of the path, return the element
            return ip;
        } else {
            // The path has more segments, keep recursing
            return ip->getParam(oid, status);
        }
    }

    /**
     * @brief gets the child parameter by name
     * @tparam Struct the type of the struct that we are getting the child parameter from
     * @param fields the tuple of FieldInfo structs that describe the fields of the struct
     * @param name the name of the child parameter
     * @return a unique pointer to the child parameter, or nullptr if it does not exist
     * 
     * This function is a helper function for getParam(Path& oid, U& value) that finds the child parameter by name.
     * 
     * This intermediate function in needed to create the index sequence for the given tuple. 
     * findParamByNameImpl uses the index sequence to deduce its template parameters. 
     */
    template <typename Struct, typename Tuple>
    std::unique_ptr<IParam>  findParamByName_(const Tuple& fields, const std::string& name) {
        return findParamByNameImpl_<Struct>(fields, name, std::make_index_sequence<std::tuple_size_v<Tuple>>());
    }

    /**
     * @brief gets the child parameter by name
     * @tparam Struct the type of the struct that we are getting the child parameter from
     * @tparam Tuple the tuple of FieldInfo structs that describe the fields of the struct
     * @tparam Is the indices of the fields in the tuple
     * @param fields the tuple of FieldInfo structs that describe the fields of the struct
     * @param name the name of the child parameter
     * @return a unique pointer to the child parameter, or nullptr if it does not exist
     * 
     * This function is a helper function for findParamByName that finds the child parameter by name.
     */
    template <typename Struct, typename Tuple, std::size_t... Is>
    std::unique_ptr<IParam>  findParamByNameImpl_(const Tuple& fields, const std::string& name, std::index_sequence<Is...>) {
        /**
         * Creates an array of unique pointers to IParam objects. The pointer will be nullptr if the field name does not match 
         * the name parameter. If the field name does match the name parameter, the pointer will be a unique pointer to an 
         * IParam constructed with the field info.
         */
        std::unique_ptr<IParam> params[] = {std::get<Is>(fields).name == name ? getParamAtIndex_<Is>(fields) : nullptr ...};
        for (auto& param : params) {
            if (param) {
                // returns first IParam unique_ptr element that isn't a nullptr
                return std::move(param);
            }
        }
        return nullptr;
    }

    /**
     * @brief Create a new paramWithValue object for the field at index I
     * @tparam I the index of the field in the tuple
     * @param tuple the tuple of FieldInfo structs that describe the fields of the struct
     * @return a unique pointer to the new IParam object
     * 
     * This function is a helper function for findParamByName that creates a new ParamWithValue object for the field at 
     * index I of the tuple.
     */
    template <std::size_t I, typename Tuple>
    std::unique_ptr<IParam> getParamAtIndex_(const Tuple& tuple) {
        // Get the type of the field at index I so that we can create a ParamWithValue object of that type
        using FieldType = typename std::tuple_element_t<I, Tuple>::Field;
        return std::make_unique<ParamWithValue<FieldType>>(std::get<I>(tuple), value_.get(), descriptor_);
    }

  public:
    /**
     * @brief add a child parameter
     */
    void addParam(const std::string& oid, ParamDescriptor* param) {
        descriptor_.addSubParam(oid, param);
    }

    /**
     * @brief get a constraint by oid
     */
    const catena::common::IConstraint* getConstraint() const override {
        return descriptor_.getConstraint();
    }

    /**
     * @brief get the parameter scope
     */
    const std::string getScope() const override {
        return descriptor_.getScope();
    }

  private:
    ParamDescriptor& descriptor_;
    std::reference_wrapper<T> value_;
};

template<typename T>
inline T& getParamValue(catena::common::IParam* param) {
  return dynamic_cast<ParamWithValue<T>*>(param)->get();
}

} // namespace common
} // namespace catena
