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
 * @file ParamDescriptor.h
 * @brief Interface for accessing parameter descriptions
 * @author John R. Naylor
 * @date 2024-08-20
 */

#pragma once

//common
#include <Tags.h>
#include <IConstraint.h>
#include <PolyglotText.h>
#include <IParamDescriptor.h>
#include <IDevice.h>
#include <Authorization.h>  

#include <vector>
#include <string>
#include <unordered_map>

namespace catena {
namespace common {

class Authorizer; // forward declaration
class IDevice; // forward declaration

/**
 * @brief ParamDescriptor provides information about a parameter
 */
class ParamDescriptor : public IParamDescriptor {
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
     * @param template_oid the parameter's template oid
     * @param constraint the parameter's constraint
     * @param isCommand the parameter's command status
     * @param minimalSet the parameter's minimal set status
     * @param dm the device that the parameter belongs to
     * @param parent the parent parameter
     */
    ParamDescriptor(
      catena::ParamType type, 
      const OidAliases& oid_aliases, 
      const PolyglotText::ListInitializer name, 
      const std::string& widget,
      const std::string& scope, 
      bool read_only, 
      const std::string& oid, 
      const std::string& template_oid,
      catena::common::IConstraint* constraint,
      bool isCommand,
      IDevice& dm,
      uint32_t max_length,
      std::size_t total_length,
      bool minimal_set,
      IParamDescriptor* parent)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget},
        scope_{scope}, read_only_{read_only}, template_oid_{template_oid},
        constraint_{constraint}, isCommand_{isCommand}, dev_{dm},
        max_length_{max_length}, total_length_{total_length},
        minimal_set_{minimal_set}, parent_{parent} {
      setOid(oid);
      if (parent_ != nullptr) {
        parent_->addSubParam(oid, this);
      }
    }

    /**
     * @brief get the parameter type
     */
    ParamType type() const override { return type_; }

    /**
     * @brief get the parameter name
     */
    const PolyglotText::DisplayStrings& name() const override { return name_.displayStrings(); }

    /**
     * @brief get the parameter oid
     */
    const std::string& getOid() const override { return oid_; }

    /**
     * @brief set the parameter oid
     */
    void setOid(const std::string& oid) override { oid_ = oid; }


    /**
     * @brief return true if the parameter has a template oid
     */
    bool hasTemplateOid() const override { return !template_oid_.empty(); };

    /**
     * @brief get the parameter's template oid
     */
    const std::string& templateOid() const override { return template_oid_;};

    /**
     * @brief return the readOnly status of the parameter
     */
    inline bool readOnly() const override { return read_only_; }

    /**
     * @brief set the readOnly status of the parameter
     */
    inline void readOnly(bool flag) override { read_only_ = flag; }

    /**
     * @brief get the access scope of the parameter
     */
    const std::string& getScope() const override;

    /**
     * @brief get the minimal set status of the parameter
     */
    inline bool minimalSet() const override { return minimal_set_; }

    /**
     * @brief set the minimal set status of the parameter
     */
    inline void setMinimalSet(bool flag) override { minimal_set_ = flag; }

    /**
     * @brief Returns the max length of the array/string parameter. If max
     * length is not set in the .JSON file, then the default value of 1024 is
     * used. The default value can also be configured with the command line
     * argument "--default_max_length=#".
     * @returns max_length_
     */
    uint32_t max_length() const override;
    /**
     * @brief Returns the total length of the string_array parameter. If max
     * length is not set in the .JSON file, then the default value of 1024 is
     * used. The default value can also be configured with the command line
     * argument "--default_max_length=#".
     * @returns total_length_
     */
    std::size_t total_length() const override;

    /**
     * @brief serialize param meta data in to protobuf message
     * @param param the protobuf message to serialize to
     * @param authz the authorization information
     * 
     * this function will populate all non-value fields of the protobuf param message 
     * with the information from the ParamDescriptor
     * 
     */
    void toProto(catena::Param &param, Authorizer& authz) const override;


    /**
     * @brief serialize param meta data in to protobuf message
     * @param paramInfo the protobuf message to serialize to
     * @param authz the authorization information
     * 
     * this function will populate all non-value fields of the protobuf param message 
     * with the information from the ParamDescriptor
     */
    void toProto(catena::ParamInfo &paramInfo, Authorizer& authz) const override;

    /**
     * @brief get the parameter name by language
     * @param language the language to get the name for
     * @return the name in the specified language, or an empty string if the language is not found
     */
    const std::string& name(const std::string& language) const override;

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    void addSubParam(const std::string& oid, IParamDescriptor* item) override {
      subParams_[oid] = item;
    }

    /**
     * @brief gets a sub parameter's paramDescriptor
     * @return ParamDescriptor of the sub parameter
     */
    IParamDescriptor& getSubParam(const std::string& oid) const override {
      return *subParams_.at(oid);
    }


    /**
     * @brief return all sub parameters
     * @return a map of all sub parameters
     */
    const std::unordered_map<std::string, IParamDescriptor*>& getAllSubParams() const override {
        return subParams_;
    }

    /**
     * @brief get a constraint by oid
     */
    const catena::common::IConstraint* getConstraint() const override {
      return constraint_;
    }

    /**
     * @brief CommandResponder is a coroutine that allows commands to return
     * multiple responses throughout its execution's lifetime.
     */
    class CommandResponder : public ICommandResponder {
      public:
        /** 
         * @brief Defines the execution behaviour of the coroutine
         */
        struct promise_type {
            /**
             * @brief Creates the coroutine handle when created.
             */
            inline CommandResponder get_return_object() { 
              return CommandResponder(std::coroutine_handle<promise_type>::from_promise(*this)); 
            }
            /**
             * @brief Pauses the coroutine after creation until getNext() is
             * called.
             */
            inline std::suspend_always initial_suspend() { return {}; }
            /**
             * @brief Pauses the coroutine before destruction.
             */
            inline std::suspend_always final_suspend() noexcept { return {}; }
            /**
             * @brief Returns a CommandResponse object when co_yield is called
             * and suspends the coroutine until getNext() is called.
             */
            inline std::suspend_always yield_value(catena::CommandResponse& component) { 
              responseMessage = component;
              return {}; 
            }
            /**
             * @brief Finishes the coroutine and returns a CommandResponse.
             */
            inline void return_value(catena::CommandResponse component) { this->responseMessage = component; }
            /**
             * @brief handles exceptions that occur during execution.
             */
            inline void unhandled_exception() {
              exception_ = std::current_exception();
            }
            /**
             * @brief Rethrows exception caught by unhandled_exception().
             */
            inline void rethrow_if_exception() {
              if (exception_) std::rethrow_exception(exception_);
            }

            catena::CommandResponse responseMessage{};
            std::exception_ptr exception_; 
        };

        CommandResponder(std::coroutine_handle<promise_type> h) : handle_(h) {}
        CommandResponder(const CommandResponder&) = delete;
        CommandResponder& operator=(const CommandResponder&) = delete;
        CommandResponder(CommandResponder&& other) : handle_(other.handle_) { other.handle_ = nullptr; }
        CommandResponder& operator=(CommandResponder&& other) { 
          if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_; 
            other.handle_ = nullptr; 
          }
          return *this; 
        } 
        ~CommandResponder() { 
          if (handle_) handle_.destroy();  
        }

        /**
         * @brief Returns true if the coroutine has not finished execution.
         */
        inline bool hasMore() const override { return handle_ && !handle_.done(); }
        /**
         * @brief Resumes the coroutine and returns a CommandResponse object.
         */
        catena::CommandResponse getNext() override {
          if (hasMore()) {
            handle_.resume();
            handle_.promise().rethrow_if_exception();
          }
          return std::move(handle_.promise().responseMessage); 
        }

        private:
          std::coroutine_handle<promise_type> handle_;
    };

    /**
     * @brief define the command implementation
     * @param commandImpl a function that takes a Value and returns a CommandResponder
     * 
     * The passed function will be executed when executeCommand is called on this param object.
     * If this is not a command parameter, an exception will be thrown.
     */
    void defineCommand(std::function<std::unique_ptr<ICommandResponder>(catena::Value)> commandImpl) override {
      if (!isCommand_) {
        throw std::runtime_error("Cannot define a command on a non-command parameter");
      }
      commandImpl_ = commandImpl;
    }

    /**
     * @brief execute the command
     * @param value the value to pass to the command implementation
     * @return the responser from the command implementation
     * 
     * if executeCommand is called for a command that has not been defined, then the returned
     * command response will be an exception with type UNIMPLEMENTED
     */
    std::unique_ptr<ICommandResponder> executeCommand(catena::Value value) override {
      return commandImpl_(value);
    }

    /**
     * @brief return true if this is a command parameter
     */
    inline bool isCommand() const override { return isCommand_; }

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::string widget_;
    std::string scope_;
    bool read_only_;

    std::unordered_map<std::string, IParamDescriptor*> subParams_;
    std::unordered_map<std::string, catena::common::IParam*> commands_;
    common::IConstraint* constraint_;
    uint32_t max_length_;
    std::size_t total_length_;
    
    std::string oid_;
    std::string template_oid_;
    IParamDescriptor* parent_;
    std::reference_wrapper<IDevice> dev_;

    bool isCommand_;
    bool minimal_set_;

    // default command implementation
    std::function<std::unique_ptr<ICommandResponder>(catena::Value)> commandImpl_ = [](catena::Value value) -> std::unique_ptr<ICommandResponder> { 
      return std::make_unique<CommandResponder>([value]() -> CommandResponder {
        catena::CommandResponse response;
        response.mutable_exception()->set_type("UNIMPLEMENTED");
        response.mutable_exception()->mutable_error_message()->mutable_display_strings()->insert({"en", "Command not implemented"});
        co_return response;
      }());
    };
};

}  // namespace common
}  // namespace catena
