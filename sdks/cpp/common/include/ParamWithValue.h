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
#include <IParamDescriptor.h>
#include <IDevice.h>
#include <StructInfo.h>
#include <PolyglotText.h>
#include <Authorization.h>

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
    using TSizeTracker = std::vector<std::size_t>;

    ParamWithValue() = delete;

    /**
     * @brief Construct a new ParamWithValue object and add it to the device
     */
    ParamWithValue(
        T& value,
        IParamDescriptor& descriptor,
        IDevice& dev,
        bool isCommand
    ) : value_{value}, descriptor_{descriptor} {
        if (isCommand) {
            dev.addItem(descriptor.getOid(), this);
        } else {
            dev.addItem(descriptor.getOid(), this);
        }
        initializeTracker_(value_.get());
    }

    /**
     * @brief Construct a new ParamWithValue object without adding it to device
     */
    ParamWithValue(
        T& value,
        IParamDescriptor& descriptor
    ) : value_{value}, descriptor_{descriptor} {
        initializeTracker_(value_.get());
    }

    /**
     * @brief Construct a new ParamWithValue object without adding it to device
     */
    ParamWithValue(
        T& value,
        IParamDescriptor& descriptor,
        std::shared_ptr<std::size_t> mSizeTracker,
        std::shared_ptr<TSizeTracker> tSizeTracker
    ) : value_{value}, descriptor_{descriptor} {
        mSizeTracker_ = mSizeTracker;
        tSizeTracker_ = tSizeTracker;
    }

    /**
     * @brief Construct a new ParamWithValue object using FieldInfo
     */
    template <typename FieldType = T, typename ParentType>
    ParamWithValue(
        const FieldInfo<FieldType, ParentType>& field, 
        ParentType& parentValue,
        IParamDescriptor& parentDescriptor
    ) : descriptor_{parentDescriptor.getSubParam(field.name)}, value_{(parentValue.*(field.memberPtr))} {
        initializeTracker_(value_.get());
    }

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

  public:
    /**
     * @brief creates a shallow copy the parameter
     * 
     * Needed to copy IParam objects
     * 
     * ParamWithValue objects only contain two references, so they are cheap to copy
     */
    std::unique_ptr<IParam> copy() const override {
        return std::make_unique<ParamWithValue<T>>(value_.get(), descriptor_, mSizeTracker_, tSizeTracker_);
    }

    /**
     * @brief serialize the parameter value to protobuf if authorized
     * @param value the protobuf value to serialize to
     * @param clientScope the client scope
     */
    catena::exception_with_status toProto(catena::Value& value, Authorizer& authz) const override {
        catena::exception_with_status ans{"", catena::StatusCode::OK};
        if (!authz.readAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to read param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        } else {
            catena::common::toProto<T>(value, &value_.get(), descriptor_, authz);
        }
        return ans;
    }

    /**
     * @brief serialize the parameter descriptor to protobuf
     * include both the descriptor and the value
     * @param param the protobuf value to serialize to
     * @param clientScope the client scope
     */
    catena::exception_with_status toProto(catena::Param& param, Authorizer& authz) const override {
        catena::exception_with_status ans{"", catena::StatusCode::OK};
        if (!authz.readAuthz(*this)) {
            return catena::exception_with_status("Not authorized to read param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        } else {
            descriptor_.toProto(param, authz);
            catena::common::toProto<T>(*param.mutable_value(), &value_.get(), descriptor_, authz);
        }
        return ans;
    }

    /**
     * @brief serialize the parameter descriptor to protobuf
     * include both the descriptor and the value
     * @param paramInfo the protobuf value to serialize to
     * @param authz the authorization information
     */
    catena::exception_with_status toProto(catena::ParamInfoResponse& paramInfo, Authorizer& authz) const override {
        catena::exception_with_status ans{"", catena::StatusCode::OK};
        if (!authz.readAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to read param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        } else {
            descriptor_.toProto(*paramInfo.mutable_info(), authz);
        }
        return ans;
    }


    /**
     * @brief deserialize the parameter value from protobuf if authorized
     * @param value the protobuf value to deserialize from
     * @param clientScope the client scope
     */
    catena::exception_with_status fromProto(const catena::Value& value, Authorizer& authz) override {
        catena::exception_with_status ans{"", catena::StatusCode::OK};
        if (!authz.readAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to read param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        }
        else if (!authz.writeAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to write to param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        } else {
            catena::common::fromProto<T>(value, &value_.get(), descriptor_, authz);
        }
        return ans;
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
    std::unique_ptr<IParam> getParam(Path& oid, Authorizer& authz, catena::exception_with_status& status) override {
        return getParam_(oid, value_.get(), authz, status);
    }

    /**
     * @brief define the command implementation
     * @param commandImpl a function that takes a Value and returns a CommandResponder
     */ 
    void defineCommand(std::function<std::unique_ptr<IParamDescriptor::ICommandResponder>(catena::Value)> commandImpl) {
        descriptor_.defineCommand(commandImpl);
    }

    /**
     * @brief execute the command for the parameter
     * @param value the value to pass to the command implementation
     * @return the responser from the command implementation
     */
    std::unique_ptr<IParamDescriptor::ICommandResponder> executeCommand(const catena::Value& value) const override {
        return descriptor_.executeCommand(value);
    }

    /**
     * @brief Gets the size of the array parameter.
     * @return The size of the array parameter, or 0 if the parameter is not an
     * array.
     */
    uint32_t size() const override {     
        return size(value_.get());
    }

    /**
     * @brief Adds an empty element to the end of an array parameter.
     * @param authz The Authorizer to test write permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return A unique ptr to the new element, or nullptr if the operation
     * failed.
     */
    std::unique_ptr<IParam> addBack(Authorizer& authz, catena::exception_with_status& status) override {     
        return addBack(value_.get(), authz, status);
    }

    /**
     * @brief Removes the last element from an array parameter.
     * @param authz The Authorizer to test write permissions with.
     * @return OK if succcessful, otherwise an error.
     */
    catena::exception_with_status popBack(Authorizer& authz) override {
        return popBack(value_.get(), authz);
    }

    /**
     * @brief get the descriptor of the parameter
     * @return the descriptor of the parameter
     */
    const IParamDescriptor& getDescriptor() const override {
        return descriptor_;
    }

    /**
     * @brief Check if the parameter is an array type
     * @return true if the parameter is an array type
     */
    bool isArrayType() const override {
        return (type().value() == catena::ParamType::STRUCT_ARRAY ||
                type().value() == catena::ParamType::INT32_ARRAY ||
                type().value() == catena::ParamType::FLOAT32_ARRAY ||
                type().value() == catena::ParamType::STRING_ARRAY ||
                type().value() == catena::ParamType::STRUCT_VARIANT_ARRAY);
    }

  private:
    /**
     * @brief Gets the size of the array parameter.
     * @return 0.
     * 
     * This generic template is used when the type is not a CatenaStructArray.
     * Since there is no .size() attribute, it only returns 0.
     */
    template <typename U>
    uint32_t size(U& value) const {     
        return 0;
    }

    /**
     * @brief Gets the size of the array parameter.
     * @return The size of the array parameter.
     * 
     * This specialization is used when the type is a CatenaStructArray.
     */
    template<meta::IsVector U>
    uint32_t size(U& value) const {     
        return value.size();
    }

    /**
     * @brief Adds an empty element to the end of an array parameter.
     * @tparam U The type of the value that we are adding to the back of.
     * @param value The value that we are adding to the back of.
     * @param authz The Authorizer to test write permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return nullptr.
     * 
     * This generic template is used when the type is not a CatenaStructArray.
     * Since you cant add to the back, it only returns nullptr.
     */
    template <typename U>
    std::unique_ptr<IParam> addBack(U& value, Authorizer& authz, catena::exception_with_status& status) {
        status = catena::exception_with_status("Cannot add generic type to param " + descriptor_.getOid(), catena::StatusCode::INVALID_ARGUMENT);
        return nullptr;
    }
    
    /**
     * @brief Adds an empty element to the end of an array parameter.
     * @tparam U the type of the value that we are adding to the back of.
     * @param value The value that we are adding to the back of.
     * @param authz The Authorizer to test write permissions with.
     * @param status The status of the operation. OK if successful, otherwise
     * an error.
     * @return A unique ptr to the new element, or nullptr if the operation
     * failed.
     * 
     * This specialization is used when the type is a CatenaStructArray.
     */
    template<meta::IsVector U>
    std::unique_ptr<IParam> addBack(U& value, Authorizer& authz, catena::exception_with_status& status) {
        using ElemType = U::value_type;
        auto oidIndex = value.size();
        if (!authz.writeAuthz(*this)) {
            status = catena::exception_with_status("Not authorized to write to param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
            return nullptr;
        } else if (oidIndex >= descriptor_.max_length()) {
            status = catena::exception_with_status("Array " + descriptor_.getOid() + " at maximum capacity", catena::StatusCode::OUT_OF_RANGE);
            return nullptr;
        } else {
            value.push_back(ElemType());
            return std::make_unique<ParamWithValue<ElemType>>(value[oidIndex], descriptor_);
        }
    }

    /**
     * @brief Removes the last element from an array parameter.
     * @tparam U the type of the value that we are poping the back of.
     * @param value The value that we are poping the back of.
     * @param authz The Authorizer to test write permissions with.
     * @return catena::exception_with_status error.
     * 
     * This generic template is used when the type is not a CatenaStructArray.
     * Since you cant pop the back, it only returns an error.
     */
    template <typename U>
    catena::exception_with_status popBack(U& value, Authorizer& authz) {
        // This type is not a CatenaStruct or CatenaStructArray so it has no sub-params
        return catena::exception_with_status("Cannot pop generic type", catena::StatusCode::INVALID_ARGUMENT);
    }

    /**
     * @brief Removes the last element from an array parameter.
     * @tparam U the type of the value that we are poping the back of.
     * @param value The value that we are poping the back of.
     * @param authz The Authorizer to test write permissions with.
     * @return OK if successful, error otherwise.
     * 
     * This specialization is used when the type is a CatenaStructArray.
     */
    template<meta::IsVector U>
    catena::exception_with_status popBack(U& value, Authorizer& authz) {
        catena::exception_with_status ans = catena::exception_with_status("", catena::StatusCode::OK);
        if (!authz.writeAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to write to param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        } else if (value.size() <= 0) {
            ans = catena::exception_with_status("Index out of bounds", catena::StatusCode::OUT_OF_RANGE);
        } else {
            value.pop_back();
        }
        return ans;
    }

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
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, Authorizer& authz, catena::exception_with_status& status) {
        // This type is not a CatenaStruct or CatenaStructArray so it has no sub-params
        status = catena::exception_with_status("No sub-params for this generic type", catena::StatusCode::INVALID_ARGUMENT);
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
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, Authorizer& authz, catena::exception_with_status& status) {
        using ElemType = U::value_type;
        if (!oid.front_is_index()) {
            status = catena::exception_with_status("Expected index in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        size_t oidIndex = oid.front_as_index();
        oid.pop();

        if (oidIndex >= value.size() || oidIndex == catena::common::Path::kEnd) {
            // If index is out of bounds, return nullptr
            status = catena::exception_with_status("Index out of bounds in path " + oid.fqoid(), catena::StatusCode::OUT_OF_RANGE);
            return nullptr;
        }

        if (oid.empty()) {
            // we reached the end of the path, return the element
            return std::make_unique<ParamWithValue<ElemType>>(value[oidIndex], descriptor_);
        } else {
            if constexpr (CatenaStruct<ElemType> || meta::IsVariant<ElemType>) {
                // The path has more segments, keep recursing
                return ParamWithValue<ElemType>(value[oidIndex], descriptor_).getParam(oid, authz, status);
            } else {
                // This type is not a CatenaStructArray so it has no sub-params
                status = catena::exception_with_status("Param " + oid.fqoid() + " does not exist", catena::StatusCode::NOT_FOUND);
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
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, Authorizer& authz, catena::exception_with_status& status) {
        if (!oid.front_is_string()) {
            status = catena::exception_with_status("Expected string in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        std::string oidStr = oid.front_as_string();
        oid.pop();
        auto fields = StructInfo<U>::fields; // get tuple of FieldInfo for this struct type

        std::unique_ptr<IParam> ip = findParamByName_<U>(fields, oidStr);
        if (!ip) {
            status = catena::exception_with_status("Param " + oid.fqoid() + " does not exist", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }

        if (oid.empty()) {
            // we reached the end of the path, return the element
            return ip;
        } else {
            // The path has more segments, keep recursing
            return ip->getParam(oid, authz, status);
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

    template <meta::IsVariant U>
    std::unique_ptr<IParam> getParam_(Path& oid, U& value, Authorizer& authz, catena::exception_with_status& status) {
        if (!oid.front_is_string()) {
            status = catena::exception_with_status("Expected string in path " + oid.fqoid(), catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }
        std::string oidStr = oid.front_as_string();
        oid.pop();   
        if (catena::common::alternativeNames<U>[value.index()] != oidStr) {
            status = catena::exception_with_status("Param " + oid.fqoid() + " does not exist", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }

        if (oid.empty()) {
            // we reached the end of the path, return the element
            return std::visit([&](auto& arg) -> std::unique_ptr<IParam> {
                using V = std::decay_t<decltype(arg)>;
                return std::make_unique<ParamWithValue<V>>(arg, descriptor_.getSubParam(oidStr));
            }, value);
        } else {
            // The path has more segments, keep recursing
            return std::visit([&](auto& arg) -> std::unique_ptr<IParam> {
                using V = std::decay_t<decltype(arg)>;
                return ParamWithValue<V>(arg, descriptor_.getSubParam(oidStr)).getParam(oid, authz, status);
            }, value);
        }
    }


  public:
    /**
     * @brief add a child parameter
     */
    void addParam(const std::string& oid, IParamDescriptor* param) {
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
    const std::string& getScope() const override {
        return descriptor_.getScope();
    }

  private: // Tracker functions.
    template<typename U>
    /**
     * @brief Helper function to return length of value if it's a string.
     * @returns The length of value if it's a string, or 0 otherwise.
     */
    std::size_t str_length(const U& value) const { return 0; }
    std::size_t str_length(const std::string& value) const { return value.length(); }
    // Tracker initializers. One overload for each case.
    /**
     * @brief Default initializeTracker_(). Sets mSizeTracker to length of value
     * if it's a string
     * @returns catena::exception_with_status OK.
     */
    template<typename U>
    catena::exception_with_status initializeTracker_(const U& value) {
        catena::exception_with_status ans{"OK", catena::StatusCode::OK};
        try {
            // 0 unless value is a string.
            *mSizeTracker_ = str_length(value);
        } catch (...) {
            ans = catena::exception_with_status("Could not initialize tracker for " + descriptor_.getOid(), catena::StatusCode::INTERNAL);
        }
        return ans;
    }
    /**
     * @brief std::vector overload of initializeTracker_(). Sets mSizeTracker to
     * the number of elements in arrayValue and tSizeTracker to the total
     * length of all values in arrayValue if arrayValue is a string array.
     * @param arrayValue The param's vector value.
     * @returns catena::exception_with_status.
     */
    template<meta::IsVector U>
    catena::exception_with_status initializeTracker_(const U& arrayValue) {
        catena::exception_with_status ans{"OK", catena::StatusCode::OK};
        try {
            *mSizeTracker_ = arrayValue.size();
            // STRING_ARRAY only functionality.
            if (descriptor_.type() == catena::ParamType::STRING_ARRAY) {
                // Initializing/clearing the tSizeTracker.
                if (!tSizeTracker_) {
                    tSizeTracker_ = std::make_shared<TSizeTracker>();
                } else {
                    tSizeTracker_->clear();
                }
                // Setting mSizeTracker and tSizeTracker.
                for (auto& value : arrayValue) {
                    tSizeTracker_->push_back(str_length(value));
                }
            }
        } catch (...) {
            ans = catena::exception_with_status("Could not initialize tracker for " + descriptor_.getOid(), catena::StatusCode::INTERNAL);
        }
        return ans;
    }

    // Tracker updaters. One overload for each case.
    /**
     * @brief Default set value overload of updateTracker_. Makes a call to
     * initializeTracker_ requirements are met.
     * @param oldValue The paramWithValue's existing value.
     * @param newValue The value to replace oldValue with. Must be the same
     * type as oldValude.
     * @param index The index to insert the value at. Must be Path::kNone.
     * @returns catena::exception_with_status.
     */
    template<typename U, typename V>
    catena::exception_with_status updateTracker_(U& oldValue, const V& newValue, Path::Index index) {
        catena::exception_with_status ans{"OK", catena::StatusCode::OK};
        // Index cannot be defined.
        if (index != Path::kNone) {
            ans = catena::exception_with_status("Index not applicable to setValue for param " + descriptor_.getOid(), catena::StatusCode::INVALID_ARGUMENT);
        // newValue must be the same type at oldValue.
        } else if (!std::is_same<U, V>::value) {
            ans = catena::exception_with_status("Type mismatch between setValue and param " + descriptor_.getOid(), catena::StatusCode::INVALID_ARGUMENT);
        // Calling initializeTracker_ if valid input.
        } else {
            ans = initializeTracker_(newValue);
        }
        return ans;
    }
    /**
     * @brief Insert/append value to array overload of updateTracker_, updates
     * mSizeTracker accordingly, and tSizeTracker if string array.
     * @param arrayValue The paramWithValue's existing array.
     * @param value The value to insert/append. Must be the same type as
     * arrayValue's contents.
     * @param index The index to insert the value at. Cannot be Path::kNone and
     * must be within bounds.
     * @returns catena::exception_with_status.
     */
    template<typename U, typename V> requires (!meta::IsVector<V>)
    catena::exception_with_status updateTracker_(std::vector<U>& arrayValue, const V& value, Path::Index index) {
        catena::exception_with_status ans{"OK", catena::StatusCode::OK};
        // Index must be specified.
        if (index == Path::kNone) {
            ans = catena::exception_with_status("Index not specified in SetValue call to " + descriptor_.getOid(), catena::StatusCode::INVALID_ARGUMENT);
        // Value must be the same type as what's stored in the array.
        } else if (!std::is_same<U, V>::value) {
            ans = catena::exception_with_status("Type mismatch between setValue and param " + descriptor_.getOid(), catena::StatusCode::INVALID_ARGUMENT);
        // Index must be within bounds.
        } else if (index >= *mSizeTracker_ && index != Path::kEnd) {
            ans = catena::exception_with_status("Index out of bounds of array " + descriptor_.getOid(), catena::StatusCode::OUT_OF_RANGE);
        // Case 1: Appending to the end.
        } else if (index == Path::kEnd) {
            *mSizeTracker_ += 1;
            if (descriptor_.type() == catena::ParamType::STRING_ARRAY) {
                tSizeTracker_->push_back(str_length(value));
            }
        // Case 2: Inserting (do nothing unless string array).
        } else if (descriptor_.type() == catena::ParamType::STRING_ARRAY) {
            tSizeTracker_->at(index) = str_length(value);
        }
        return ans;
    }
    using Kind = catena::Value::KindCase;
    /**
     * @brief A map to help routing to the updateTracker_ functions above.
     * Protobuf is great in that you need to use a seperate function to get
     * each different type of value.
     * @param key The value's kind_case().
     * @param value The value we want passed into updateTracker_.
     * @param index The index to update (or nullptr if none).
     * @returns catena::exception_with_status.
     */
    const std::unordered_map<Kind, std::function<catena::exception_with_status(const catena::Value&, Path::Index)>> updateTrackerMap_ {
        {Kind::kInt32Value, [this](const catena::Value& value, Path::Index index) {
            return this->updateTracker_(this->get(), value.int32_value(), index);
        }},
        {Kind::kFloat32Value, [this](const catena::Value& value, Path::Index index) {
            return this->updateTracker_(this->get(), value.float32_value(), index);
        }},
        {Kind::kStringValue, [this](const catena::Value& value, Path::Index index) {
            return this->updateTracker_(this->get(), value.string_value(), index);
        }},
        {Kind::kStructValue, [this](const catena::Value& value, Path::Index index) {
            return this->updateTracker_(this->get(), value.struct_value(), index);
        }},
        {Kind::kInt32ArrayValues, [this](const catena::Value& value, Path::Index index) {
            auto ints = value.int32_array_values().ints();
            auto protoVector = std::vector<int>(ints.begin(), ints.end());
            return this->updateTracker_(this->get(), protoVector, index);
        }},
        {Kind::kFloat32ArrayValues, [this](const catena::Value& value, Path::Index index) {
            auto floats = value.float32_array_values().floats();
            auto protoVector = std::vector<float>(floats.begin(), floats.end());
            return this->updateTracker_(this->get(), protoVector, index);
        }},
        {Kind::kStringArrayValues, [this](const catena::Value& value, Path::Index index) {
            auto strings = value.string_array_values().strings();
            auto protoVector = std::vector<std::string>(strings.begin(), strings.end());
            return this->updateTracker_(this->get(), protoVector, index);
        }},
        {Kind::kStructArrayValues, [this](const catena::Value& value, Path::Index index) {
            auto structs = value.struct_array_values().struct_values();
            auto protoVector = std::vector<catena::StructValue>(structs.begin(), structs.end());
            return this->updateTracker_(this->get(), protoVector, index);
        }},
        {Kind::kStructVariantArrayValues, [this](const catena::Value& value, Path::Index index) {
            auto structs = value.struct_variant_array_values().struct_variants();
            auto protoVector = std::vector<catena::StructVariantValue>(structs.begin(), structs.end());
            return this->updateTracker_(this->get(), protoVector, index);
        }}
    };

  public: // Validation functions.
    /**
     * @brief Validates a setValue operation without changing the param's value.
     * @param value The value we want to set the param to.
     * @param index The index of the subparam to set (or nullptr if none).
     * @param ans Catena::exception_with_status output.
     * @returns true if valid.
     */
    bool validateSetValue(const catena::Value& value, Path::Index index, Authorizer& authz, catena::exception_with_status& ans) override {
        // Checking read permissions.
        if (!authz.readAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to read the param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        }
        // Checking write permissions.
        else if (!authz.writeAuthz(*this)) {
            ans = catena::exception_with_status("Not authorized to write to param " + descriptor_.getOid(), catena::StatusCode::PERMISSION_DENIED);
        // Currently: If unimplemented ignore.
        } else if (updateTrackerMap_.contains(value.kind_case())) {
            // Updating trackers.
            ans = updateTrackerMap_.at(value.kind_case())(value, index);
            // If the above was successful, check mSize and tSize.
            if (ans.status == catena::StatusCode::OK) {
                // Validating max length.
                if (*mSizeTracker_ > descriptor_.max_length()) {
                    ans = catena::exception_with_status("Param " + descriptor_.getOid() + " exceeds maximum capacity", catena::StatusCode::OUT_OF_RANGE);
                // Validating total length if applicable.
                } else if (tSizeTracker_) {
                    std::size_t totalLength = 0;
                    for (std::size_t length : *tSizeTracker_) {
                        totalLength += length;
                    }
                    if (totalLength > descriptor_.total_length()) {
                        ans = catena::exception_with_status("String array param " + descriptor_.getOid() + " exceeds total length", catena::StatusCode::OUT_OF_RANGE);
                    }
                }
            }
        }
        return ans.status == catena::StatusCode::OK;
    }
    /**
     * @brief Resets any trackers that might have been changed in validateSetValue.
     */
    void resetValidate() override {
        // Resetting the trackers.
        std::shared_ptr<std::size_t> mSizeTracker_ = std::make_shared<std::size_t>(0);
        std::shared_ptr<TSizeTracker> tSizeTracker_ = nullptr;
        // Reinitializing the trackers with the param's actual value.
        initializeTracker_(value_.get());
    }

  private:
    IParamDescriptor& descriptor_;
    std::reference_wrapper<T> value_;

    /**
     * @brief Tracker variable used to keep track of the total size of a string
     * or array.
     * Shared ptr is used as the tracker must be shared between the parent and
     * all copies.
     */
    std::shared_ptr<std::size_t> mSizeTracker_ = std::make_shared<std::size_t>(0);
    /**
     * @brief A vector used to keep track of the string length at each index
     * of a string array. For all other types, this should be a nullptr.
     * Shared ptr is used as the tracker must be shared between the parent and
     * all copies.
     */
    std::shared_ptr<TSizeTracker> tSizeTracker_ = nullptr;
};

template<typename T>
inline T& getParamValue(catena::common::IParam* param) {
  return dynamic_cast<ParamWithValue<T>*>(param)->get();
}

}; // namespace common
}; // namespace catena
