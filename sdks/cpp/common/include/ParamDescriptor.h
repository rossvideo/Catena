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
 * @file ParamWithDescription.h
 * @brief Interface for accessing parameter descriptions
 * @author John R. Naylor
 * @date 2024-08-20
 */

//common
#include <Tags.h>
#include <IParam.h>
#include <IConstraint.h>
#include <Device.h>
#include <PolyglotText.h>

// meta
#include <meta/IsVector.h>

// protobuf interface
#include <interface/param.pb.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <cassert>

namespace catena {
namespace common {

class AuthzInfo; // forward declaration

/**
 * @brief ParamDescriptor provides information about a parameter
 */
class ParamDescriptor {
  public:
    using OidAliases = std::vector<std::string>;

  public:
    /**
     * @brief ParamDescriptor does not have a default constructor
     */
    ParamDescriptor() = delete;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    ParamDescriptor(ParamDescriptor& value) = delete;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    ParamDescriptor& operator=(const ParamDescriptor& value) = delete;

    /**
     * @brief ParamDescriptor has move semantics
     */
    ParamDescriptor(ParamDescriptor&& value) = default;

    /**
     * @brief ParamDescriptor has move semantics
     */
    ParamDescriptor& operator=(ParamDescriptor&& value) = default;

    /**
     * @brief default destructor
     */
    virtual ~ParamDescriptor() = default;

    /**
     * @brief the main constructor
     * @param type the parameter type
     * @param oid_aliases the parameter's oid aliases
     * @param name the parameter's name
     * @param widget the parameter's widget
     * @param read_only the parameter's read-only status
     * @param oid the parameter's oid
     * @param collection the parameter belongs to
     */
    ParamDescriptor(
      catena::ParamType type, 
      const OidAliases& oid_aliases, 
      const PolyglotText::ListInitializer name, 
      const std::string& widget,
      const std::string& scope, 
      bool read_only, 
      const std::string& oid, 
      catena::common::IConstraint* constraint,
      ParamDescriptor* parent,
      Device& dev,
      bool isCommand)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, scope_{scope}, read_only_{read_only},
        constraint_{constraint}, parent_{parent}, dev_{dev}, isCommand_{isCommand} {
      setOid(oid);
      if (parent_ != nullptr) {
        parent_->addSubParam(oid, this);
      }
    }

    /**
     * @brief get the parameter type
     */
    ParamType type() const { return type_; }

    /**
     * @brief get the parameter name
     */
    const PolyglotText::DisplayStrings& name() const { return name_.displayStrings(); }

    /**
     * @brief get the parameter oid
     */
    const std::string& getOid() const { return oid_; }

    /**
     * @brief set the parameter oid
     */
    void setOid(const std::string& oid) { oid_ = oid; }

    /**
     * @brief return the readOnly status of the parameter
     */
    inline bool readOnly() const { return read_only_; }

    /**
     * @brief set the readOnly status of the parameter
     */
    inline void readOnly(bool flag) { read_only_ = flag; }

    /**
     * @brief get the access scope of the parameter
     */
    const std::string getScope() const;

    /**
     * @brief serialize param meta data in to protobuf message
     * @param param the protobuf message to serialize to
     * @param auth the authorization information
     * 
     * this function will populate all non-value fields of the protobuf param message 
     * with the information from the ParamDescriptor
     * 
     */
    void toProto(catena::Param &param, AuthzInfo& auth) const;

    /**
     * @brief get the parameter name by language
     * @param language the language to get the name for
     * @return the name in the specified language, or an empty string if the language is not found
     */
    const std::string& name(const std::string& language) const;
    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    void addSubParam(const std::string& oid, ParamDescriptor* item) {
      subParams_[oid] = item;
    }

    /**
     * @brief gets a sub parameter's paramDescriptor
     * @return ParamDescriptor of the sub parameter
     */
    ParamDescriptor& getSubParam(const std::string& oid) const {
      return *subParams_.at(oid);
    }

    /**
     * @brief get a constraint by oid
     */
    const catena::common::IConstraint* getConstraint() const {
      return constraint_;
    }

    /**
     * @brief define the command implementation
     * @param commandImpl a function that takes a Value and returns a CommandResponse
     * 
     * The passed function will be executed when executeCommand is called on this param object.
     * If this is not a command parameter, an exception will be thrown.
     */
    void defineCommand(std::function<catena::CommandResponse(catena::Value)> commandImpl) {
      if (!isCommand_) {
        throw std::runtime_error("Cannot define a command on a non-command parameter");
      }
      commandImpl_ = commandImpl;
    }

    /**
     * @brief execute the command
     * @param value the value to pass to the command implementation
     * @return the response from the command implementation
     * 
     * if executeCommand is called for a command that has not been defined, then the returned
     * command response will be an exception with type UNIMPLEMENTED
     */
    catena::CommandResponse executeCommand(catena::Value value) {
      return commandImpl_(value);
    }

    /**
     * @brief return true if this is a command parameter
     */
    inline bool isCommand() const { return isCommand_; }

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::string widget_;
    std::string scope_;
    bool read_only_;
    std::unordered_map<std::string, ParamDescriptor*> subParams_;
    std::unordered_map<std::string, catena::common::IParam*> commands_;
    common::IConstraint* constraint_;
    
    std::string oid_;
    ParamDescriptor* parent_;
    std::reference_wrapper<Device> dev_;

    bool isCommand_;

    // default command implementation
    std::function<catena::CommandResponse(catena::Value)> commandImpl_ = [](catena::Value value) { 
      catena::CommandResponse response;
      response.mutable_exception()->set_type("UNIMPLEMENTED");
      response.mutable_exception()->mutable_error_message()->mutable_display_strings()->insert({"en", "Command not implemented"});
      return response;
    };
};

}  // namespace common
}  // namespace catena