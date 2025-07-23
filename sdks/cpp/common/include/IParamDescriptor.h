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
 * @file IParamDescriptor.h
 * @brief Interface class for ParamDescriptor.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/15
 */

#pragma once

//common
#include <Tags.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

namespace catena {
namespace common {

class Authorizer; // forward declaration

/**
 * @brief ParamDescriptor provides information about a parameter
 */
class IParamDescriptor {
  public:
    /**
     * @brief ParamDescriptor does not have a default constructor
     */
    IParamDescriptor() = default;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    IParamDescriptor(IParamDescriptor& value) = delete;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    IParamDescriptor& operator=(const IParamDescriptor& value) = delete;

    /**
     * @brief ParamDescriptor has move semantics
     */
    IParamDescriptor(IParamDescriptor&& value) = default;

    /**
     * @brief ParamDescriptor has move semantics
     */
    IParamDescriptor& operator=(IParamDescriptor&& value) = default;

    /**
     * @brief default destructor
     */
    virtual ~IParamDescriptor() = default;

    /**
     * @brief get the parameter type
     */
    virtual ParamType type() const = 0;

    /**
     * @brief get the parameter name
     */
    virtual const PolyglotText::DisplayStrings& name() const = 0;

    /**
     * @brief get the parameter oid
     */
    virtual const std::string& getOid() const = 0;

    /**
     * @brief set the parameter oid
     */
    virtual void setOid(const std::string& oid) = 0;


    /**
     * @brief return true if the parameter has a template oid
     */
    virtual bool hasTemplateOid() const = 0;

    /**
     * @brief get the parameter's template oid
     */
    virtual const std::string& templateOid() const = 0;

    /**
     * @brief return the readOnly status of the parameter
     */
    virtual inline bool readOnly() const = 0;

    /**
     * @brief set the readOnly status of the parameter
     */
    virtual inline void readOnly(bool flag) = 0;

    /**
     * @brief get the access scope of the parameter
     */
    virtual const std::string& getScope() const = 0;

    /**
     * @brief get the minimal set status of the parameter
     */
    virtual inline bool minimalSet() const = 0;

    /**
     * @brief set the minimal set status of the parameter
     */
    virtual inline void setMinimalSet(bool flag) = 0;

    /**
     * @brief Returns the max length of the array/string parameter. If max
     * length is not set in the .JSON file, then the default value of 1024 is
     * used. The default value can also be configured with the command line
     * argument "--default_max_length=#".
     * @returns max_length_
     */
    virtual uint32_t max_length() const = 0;
    /**
     * @brief Returns the total length of the string_array parameter. If max
     * length is not set in the .JSON file, then the default value of 1024 is
     * used. The default value can also be configured with the command line
     * argument "--default_max_length=#".
     * @returns total_length_
     */
    virtual std::size_t total_length() const = 0;

    /**
     * @brief serialize param meta data in to protobuf message
     * @param param the protobuf message to serialize to
     * @param authz the authorization information
     * 
     * this function will populate all non-value fields of the protobuf param message 
     * with the information from the ParamDescriptor
     * 
     */
    virtual void toProto(catena::Param &param, Authorizer& authz) const = 0;


    /**
     * @brief serialize param meta data in to protobuf message
     * @param paramInfo the protobuf message to serialize to
     * @param authz the authorization information
     * 
     * this function will populate all non-value fields of the protobuf param message 
     * with the information from the ParamDescriptor
     */
    virtual void toProto(catena::ParamInfo &paramInfo, Authorizer& authz) const = 0;

    /**
     * @brief get the parameter name by language
     * @param language the language to get the name for
     * @return the name in the specified language, or an empty string if the language is not found
     */
    virtual const std::string& name(const std::string& language) const = 0;

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    virtual void addSubParam(const std::string& oid, IParamDescriptor* item) = 0;

    /**
     * @brief gets a sub parameter's paramDescriptor
     * @return ParamDescriptor of the sub parameter
     */
    virtual IParamDescriptor& getSubParam(const std::string& oid) const = 0;


    /**
     * @brief return all sub parameters
     * @return a map of all sub parameters
     */
    virtual const std::unordered_map<std::string, IParamDescriptor*>& getAllSubParams() const = 0;

    /**
     * @brief get a constraint by oid
     */
    virtual const catena::common::IConstraint* getConstraint() const = 0;

    /**
     * @brief CommandResponder is a coroutine that allows commands to return
     * multiple responses throughout its execution's lifetime.
     */
    class ICommandResponder {
      public:
        ICommandResponder() = default;
        ICommandResponder(const ICommandResponder&) = delete;
        ICommandResponder& operator=(const ICommandResponder&) = delete;
        virtual ~ICommandResponder() = default;

        /**
         * @brief Returns true if the coroutine has not finished execution.
         */
        virtual inline bool hasMore() const = 0;
        /**
         * @brief Resumes the coroutine and returns a CommandResponse object.
         */
        virtual catena::CommandResponse getNext() = 0;
    };

    /**
     * @brief define the command implementation
     * @param commandImpl a function that takes a Value and returns a CommandResponse
     * 
     * The passed function will be executed when executeCommand is called on this param object.
     * If this is not a command parameter, an exception will be thrown.
     */
    virtual void defineCommand(std::function<std::unique_ptr<ICommandResponder>(const catena::Value&)> commandImpl) = 0;

    /**
     * @brief execute the command
     * @param value the value to pass to the command implementation
     * @return the response from the command implementation
     * 
     * if executeCommand is called for a command that has not been defined, then the returned
     * command response will be an exception with type UNIMPLEMENTED
     */
    virtual std::unique_ptr<ICommandResponder> executeCommand(const catena::Value& value) = 0;

    /**
     * @brief return true if this is a command parameter
     */
    virtual inline bool isCommand() const = 0;
};

}  // namespace common
}  // namespace catena
