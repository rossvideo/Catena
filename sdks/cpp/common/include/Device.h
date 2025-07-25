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
 * @file Device.h
 * @brief Device class definition
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

/**
 * @example hello_world.cpp Demonstrates creation of a simple device model, 
 * and how business logic can interact with it.
 */

// common
#include <Authorizer.h>
#include <Path.h>
#include <Enums.h>
#include <IParam.h>
#include <ILanguagePack.h>
#include <Tags.h>  
#include <Status.h>
#include <IMenu.h>
#include <IMenuGroup.h>

// Interface
#include <IDevice.h>

// protobuf interface
#include <interface/device.pb.h>

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <cassert>
#include <type_traits>
#include <coroutine>

namespace catena {
namespace common {

/**
 * @brief The default limit for param array accesses.
 */
constexpr uint32_t kDefaultMaxArrayLength{1024};

/**
 * @brief Implements the Device interface defined in the protobuf
 */
class Device : public IDevice {
  public:
    /**
     * @brief convenience type aliases to types of objects contained in the
     * device
     */
    using DetailLevel_e = catena::Device_DetailLevel;

    /**
     * @brief Construct a new Device object
     */
    Device() = default;

    /**
     * @brief Construct a new Device object
     */
    Device(uint32_t slot, Device_DetailLevel detail_level, std::vector<std::string> access_scopes,
      std::string default_scope, bool multi_set_enabled, bool subscriptions)
      : slot_{slot}, detail_level_{detail_level}, access_scopes_{access_scopes},
      default_scope_{default_scope}, multi_set_enabled_{multi_set_enabled},
	    subscriptions_{subscriptions}, default_max_length_{kDefaultMaxArrayLength},
      default_total_length_{kDefaultMaxArrayLength}  {}

    /**
     * @brief Destroy the Device object
     */
    virtual ~Device() = default;

    /**
     * @brief set the slot number of the device
     * @param slot the slot number of the device
     */
    inline void slot(const uint32_t slot) override { slot_ = slot; }

    /**
     * @brief get the slot number of the device
     * @return slot number
     */
    inline uint32_t slot() const override { return slot_; }

    /**
     * @brief Returns the device's mutex.
     */
    inline std::mutex& mutex() override { return mutex_; };

    /**
     * @brief set the detail level of the device
     * @param detail_level the detail level of the device
     */
    inline void detail_level(const DetailLevel_e detail_level) override { detail_level_ = detail_level; }

    /**
     * @brief get the detail level of the device
     * @return DetailLevel_
     */
    inline DetailLevel_e detail_level() const override { return detail_level_; }
    /**
     * @brief Returns the device's default scope.
     */
    inline const std::string& getDefaultScope() const override { return default_scope_; }

    /**
     * @brief Check if subscriptions are enabled for this device
     * @return true if subscriptions are enabled, false otherwise
     */
    inline bool subscriptions() const override { return subscriptions_; }

    /**
     * @return The default max length for this device's array params.
     */
    inline uint32_t default_max_length() const override {return default_max_length_;}
    /**
     * @return The default total length for this device's string array params.
     */
    inline uint32_t default_total_length() const override {return default_total_length_;}

    /**
     * @brief Sets the default_max_length_ for this device's array params.
     * If default_max_length <= 0, then it reverts default_max_length_ to
     * kDefaultMaxArrayLength.
     * @param default_max_length The value to set default_max_length_ to.
     */
    void set_default_max_length(const uint32_t default_max_length) override {
      default_max_length_ = default_max_length > 0 ? default_max_length : kDefaultMaxArrayLength;
    }
    /**
     * @brief Sets the default_total_length_ for this device's array params.
     * If default_total_length <= 0, then it reverts default_total_length_ to
     * kDefaultMaxArrayLength.
     * @param default_total_length The value to set default_total_length_ to.
     */
    void set_default_total_length(const uint32_t default_total_length) override {
      default_total_length_ = default_total_length > 0 ? default_total_length : kDefaultMaxArrayLength;
    }

    /**
     * @brief Create a protobuf representation of the device.
     * @param dst the protobuf representation of the device.
     * @param shallow if true, only the top-level info is copied, params,
     * commands etc are not copied. Design intent is to permit large models to
     * stream their parameters instead of sending a huge device model in one
     * big lump.
     * 
     * N.B. This method is not thread-safe. It is the caller's responsibility
     * to ensure that the device is not modified while this method is running.
     * This class provides a LockGuard helper class to make this easier.
     */
    void toProto(::catena::Device& dst, Authorizer& authz, bool shallow = true) const override;

    /**
     * @brief Create a protobuf representation of the language packs.
     * @param packs the protobuf representation of the language packs.
     */
    void toProto(::catena::LanguagePacks& packs) const override;

    /**
     * @brief Create a protobuf representation of the language list.
     * @param list the protobuf representation of the language list.
     */
    void toProto(::catena::LanguageList& list) const override;

    using ComponentLanguagePack = catena::DeviceComponent_ComponentLanguagePack;

    /**
     * @brief Returns true if device has the specified language pack.
     * @param languageId The language id of the language pack to check
     * @return True if the device has the specified language pack.
     */
    bool hasLanguage(const std::string& languageId) const override { return language_packs_.contains(languageId); }

    /**
     * @brief Adds a language pack to the device. Requires client to have
     * admin:w scope.
     * @param language The language to add to the device.
     * @param authz The authorizer object containing client's scopes.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     * Intention is for the AddLanguage RPCs / API calls to be serviced by this
     * method.
     */
    catena::exception_with_status addLanguage(catena::AddLanguagePayload& language, Authorizer& authz = Authorizer::kAuthzDisabled) override;

    /**
     * @brief Removed a language pack from the device. Requires client to have
     * admin:w scope. Fails if the language pack was shipped with the device.
     * @param language The language to remove from the device.
     * @param authz The authorizer object containing client's scopes.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     */
    catena::exception_with_status removeLanguage(const std::string& languageId, Authorizer& authz = Authorizer::kAuthzDisabled) override;

    /**
     * @brief Finds and returns a language pack based on languageId.
     * @param languageId The language id of the language pack e.g. "en"
     * @param pack Output var containing the found LanguagePack.
     * @return exception_with_status containing the status of the operation.
     */
    catena::exception_with_status getLanguagePack(const std::string& languageId, ComponentLanguagePack& pack) const override;

    /**
     * @brief DeviceSerializer is a coroutine that serializes the device into a
     * stream of DeviceComponents
     * This struct manages the state and lifetime of the coroutine. It also
     * provides the interface for resuming the coroutine.
     */
    class DeviceSerializer : public IDeviceSerializer {
      public:
        /** 
         * @brief Defines the execution behaviour of the coroutine
         * 
         * Coroutine types are required to contain a promise_type struct that
         * defines some special member functions that the compiler will call
         * when the coroutine is created, destroyed, or resumed.
         */
        struct promise_type {

            /**
             * @brief Called when the coroutine is created (i.e. when
             * getComponentSerializer() is called). It creates the handle for
             * the coroutine and returns the DeviceSerializer object.
             */
            inline DeviceSerializer get_return_object() { 
              return DeviceSerializer(std::coroutine_handle<promise_type>::from_promise(*this)); 
            }

            /**
             * @brief Called when the coroutine is created. It returns a
             * std::suspend_always awaitable so that the coroutine will be
             * immediately suspended. This means the first component will not
             * be created until getNext() is called.
             */
            inline std::suspend_always initial_suspend() { return {}; }

            /** 
             * @brief Called when the coroutine is completed. It returns a
             * std::suspend_always awaitable causing the coroutine to be
             * suspended before it destroys itself. The coroutine will be
             * destroyed when the device serializer destructor is called.
             */
            inline std::suspend_always final_suspend() noexcept { return {}; }

            /**
             * @brief Called when the coroutine reaches a co_yield statement.
             * It stores the yielded DeviceComponent then suspends the
             * coroutine.
             */
            inline std::suspend_always yield_value(catena::DeviceComponent& component) { 
              deviceMessage = component;
              return {}; 
            }

            /**
             * @brief Called when the coroutine reaches a co_return statement.
             * It stores the returned DeviceComponent. The coroutine is then
             * set as done.
             */
            inline void return_value(catena::DeviceComponent component) { this->deviceMessage = component; }

            /**
             * @brief Called if an exception is thrown in the coroutine. It
             * stores the exception so that it can be rethrown by the function
             * that resumed the coroutine.
             */
            inline void unhandled_exception() {
              exception_ = std::current_exception();
            }

            /**
             * @brief Not called by the compiler but instead is called by the
             * function that resumes the coroutine. It rethrows the exception
             * that was caught by unhandled_exception().
             */
            inline void rethrow_if_exception() {
              if (exception_) std::rethrow_exception(exception_);
            }

            catena::DeviceComponent deviceMessage{};
            std::exception_ptr exception_; 
        };

        DeviceSerializer(std::coroutine_handle<promise_type> h) : handle_(h) {}

        DeviceSerializer(const DeviceSerializer&) = delete;
        DeviceSerializer& operator=(const DeviceSerializer&) = delete;

        DeviceSerializer(DeviceSerializer&& other) : handle_(other.handle_) { other.handle_ = nullptr; }
        DeviceSerializer& operator=(DeviceSerializer&& other) { 
          if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_; 
            other.handle_ = nullptr; 
          }
          return *this; 
        } 
        
        ~DeviceSerializer() { 
          if (handle_) handle_.destroy();  
        }

        /**
         * @brief returns true if there are more DeviceComponents to be
         * serialized
         */
        inline bool hasMore() const override { return handle_ && !handle_.done(); }

        /**
         * @brief get the next DeviceComponent to be serialized.
         * @return the next DeviceComponent
         * 
         * If the coroutine is done and there are no more components to
         * serialize then an empty DeviceComponent is returned.
         */
        catena::DeviceComponent getNext() override;

      private:
        /**
         * The coroutine handle is a pointer to the coroutine state
         * 
         * The coroutine handle provides the following operations:
         * resume() - resumes the coroutine from where it was suspended
         * destroy() - destroys the coroutine
         * done() - returns true if the coroutine has completed (i.e. co_return
         * has been called)
         */
        std::coroutine_handle<promise_type> handle_;
    };
    /**
     * @brief get a serializer for the device
     * @param authz The authorizer object containing the scopes of the client
     * @param dl The detail level to retrieve information in.
     * @param subscribedOids the oids of the subscribed parameters
     * @param shallow if true, the device will be returned in parts, otherwise
     * the whole device will be returned in one message
     * @return a DeviceSerializer object
     */
    std::unique_ptr<IDeviceSerializer> getComponentSerializer(Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow = false) const override;
    /**
     * @brief This is a helper function for the shared IDevice function
     * getComponentSerializer to return a DeviceSerializer object. It can be
     * used on its own if calling from a Device object.
     * 
     * @param authz The authorizer object containing the scopes of the client
     * @param subscribedOids the oids of the subscribed parameters
     * @param dl The detail level to retrieve information in.
     * @param shallow if true, the device will be returned in parts, otherwise
     * the whole device will be returned in one message
     */
    DeviceSerializer getDeviceSerializer(Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow = false) const;
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for parameters and commands.
     * @param key item's unique key
     * @param item the item to be added
     */
    void addItem(const std::string& key, IParam* item) override {
        if (item->getDescriptor().isCommand()) {
            commands_[key] = item;
        } else {
            params_[key] = item;
        }
    }
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for constraints.
     * @param key item's unique key
     * @param item the item to be added
     */
    void addItem(const std::string& key, IConstraint* item) override { constraints_[key] = item; }
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for menu groups.
     * @param key item's unique key
     * @param item the item to be added
     */
    void addItem(const std::string& key, IMenuGroup* item) override { menu_groups_[key] = item; }
    /**
     * @brief add an item to one of the collections owned by the device
     * Overload for language packs.
     * @param key item's unique key
     * @param item the item to be added
     */
    void addItem(const std::string& key, ILanguagePack* item) override { language_packs_[key] = item; }

    /**
     * @brief gets an item from one of the collections owned by the device
     * @return nullptr if the item is not found, otherwise the item
     */
    template <typename TAG>
    typename TAG::type* getItem(const std::string& key) const {

        GET_ITEM(common::ParamTag, params_)
        GET_ITEM(common::ConstraintTag, constraints_)
        GET_ITEM(common::MenuGroupTag, menu_groups_)
        GET_ITEM(common::CommandTag, commands_)
        GET_ITEM(common::LanguagePackTag, language_packs_)
        return nullptr;
    }

    /**
     * @brief get a parameter by oid with authorization
     * @param fqoid the fully qualified oid of the parameter
     * @param authz The Authorizer to test read permission with.
     * @param status will contain an error message if the parameter does not
     * exist
     * @return a unique pointer to the parameter, or nullptr if it does not
     * exist
     * 
     * gets a parameter if it exists and the client is authorized to read it.
     */
    std::unique_ptr<IParam> getParam(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const override;

    /**
     * @brief get a parameter by oid with authorization
     * @param path the full path to the parameter
     * @param authz The Authorizer to test read permission with.
     * @param status will contain an error message if the parameter does not
     * exist
     * @return a unique pointer to the parameter, or nullptr if it does not
     * exist
     * 
     * gets a parameter if it exists and the client is authorized to read it.
     */
    std::unique_ptr<IParam> getParam(catena::common::Path& path, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const override;
    
    /**
     * @brief get all top level parameters
     * @param status the status of the operation
     * @param authz the authorizer object
     * @return a vector of unique pointers to the parameters
     */
    std::vector<std::unique_ptr<IParam>> getTopLevelParams(catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const override;


    /**
     * @brief get a command by oid
     * @param fqoid the fully qualified oid of the command
     * @param authz the authorizer object
     * @param status will contain an error message if the command does not
     * exist
     * @return a unique pointer to the command, or nullptr if it does not exist
     * @todo add authorization checking
     */
    std::unique_ptr<IParam> getCommand(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const override;

    /**
     * @brief Validates each payload in a multiSetValuePayload without commit
     * changes to param value_s.
     * @param src The MultiSetValuePayload to validate.
     * @param ans The exception_with_status to return.
     * @param authz The Authorizer to test with.
     * @returns true if the call is valid.
     */
    bool tryMultiSetValue (catena::MultiSetValuePayload src, catena::exception_with_status& ans, Authorizer& authz = Authorizer::kAuthzDisabled) override;
    
    /**
     * @brief Sets the values of a device's parameter's using a
     * MultiSetValuePayload.
     * This function assumes that tryMultiSetValue is called at some point
     * beforehand to validate the call.
     * @param src The MultiSetValuePayload to update the device with.
     * @param authz The Authroizer with the client's scopes.
     * @returns an exception_with_status with status set OK if successful.
     */
    catena::exception_with_status commitMultiSetValue (catena::MultiSetValuePayload src, Authorizer& authz) override;

    /**
     * @brief deserialize a protobuf value object into the parameter value
     * pointed to by jptr.
     * @param jptr json pointer to the part of the device model to update.
     * @param src the value to update the parameter with.
     * @param authz The Authorizer to test with.
     * @return an exception_with_status with status set OK if successful,
     * otherwise an error.
     * 
     * This method essentially redirects the input values to tryMultiSetValue()
     * and commitMultiSetValue().
     * It remains to support the old way of setting values.
     */
    catena::exception_with_status setValue (const std::string& jptr, catena::Value& src, Authorizer& authz = Authorizer::kAuthzDisabled) override;

    /**
     * @brief serialize the parameter value to protobuf
     * @param jptr json pointer to the part of the device model to serialize.
     * @param dst the protobuf value to serialize to.
     * @param authz The Authorizer to test read permission with.
     * @return an exception_with_status with status set OK if successful,
     * otherwise an error.
     * Intention is to for GetValue RPCs / API calls to be serviced by this
     * method.
     */
    catena::exception_with_status getValue (const std::string& jptr, catena::Value& value, Authorizer& authz = Authorizer::kAuthzDisabled) const override;

    /**
     * @brief check if a parameter should be sent based on detail level and
     * authorization
     * @param param the parameter to check
     * @param is_subscribed true if the parameter is subscribed, false
     * otherwise
     * @param authz the authorizer object
     * @return true if the parameter should be sent, false otherwise
     */
    bool shouldSendParam(const IParam& param, bool is_subscribed, Authorizer& authz) const override;

    /**
     * @brief get the signal emitted when a value is set by the client.
     * @return the signal
     */
    vdk::signal<void(const std::string&, const IParam*)>& getValueSetByClient() override { return valueSetByClient_; }

    /**
     * @brief get the signal emitted when a language pack is added to the device.
     * @return the signal
     */
    vdk::signal<void(const ILanguagePack*)>& getLanguageAddedPushUpdate() override { return languageAddedPushUpdate_; }

    /**
     * @brief get the signal emitted when a value is set by the server, or business
     * logic.
     * @return the signal
     */
    vdk::signal<void(const std::string&, const IParam*)>& getValueSetByServer() override { return valueSetByServer_; }
    
    /**
     * @brief get the asset request signal
     * @return the signal
     */
     vdk::signal<void(const std::string&, const Authorizer*)>& getDownloadAssetRequest() override { return downloadAssetRequest_; } 

     /**
      * @brief get the upload asset request signal
      * @return the signal
      */
     vdk::signal<void(const std::string&, const Authorizer*)>& getUploadAssetRequest() override { return uploadAssetRequest_; }

    /**
     * @brief get the delete asset request signal
     * @return the signal
     */
     vdk::signal<void(const std::string&, const Authorizer*)>& getDeleteAssetRequest() override { return deleteAssetRequest_; }
  private:

    /**
     * @brief signal emitted when a value is set by the client.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const IParam*)> valueSetByClient_;

    /**
     * @brief signal emitted when a language pack is added to the device.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const ILanguagePack*)> languageAddedPushUpdate_;

    /**
     * @brief signal emitted when a value is set by the server, or business
     * logic.
     * Intended recipient is the connection manager.
     */
    vdk::signal<void(const std::string&, const IParam*)> valueSetByServer_;

    /**
     * @brief signal emitted when a download asset request is made.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const Authorizer*)> downloadAssetRequest_;

    /**
     * @brief signal emitted when an upload asset request is made.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const Authorizer*)> uploadAssetRequest_;
    /**
     * @brief signal emitted when a delete asset request is made.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const Authorizer*)> deleteAssetRequest_;

    uint32_t slot_;
    Device_DetailLevel detail_level_;
    std::unordered_map<std::string, catena::common::IConstraint*> constraints_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, common::IMenuGroup*> menu_groups_;
    std::unordered_map<std::string, IParam*> commands_;
    std::unordered_map<std::string, common::ILanguagePack*> language_packs_;
    // Required to maintain ownership of added language packs.
    std::unordered_map<std::string, std::shared_ptr<common::ILanguagePack>> added_packs_;
    std::vector<std::string> access_scopes_;
    std::string default_scope_;
    bool multi_set_enabled_;
    bool subscriptions_;
    uint32_t default_max_length_ = 0;
    uint32_t default_total_length_ = 0;

    mutable std::mutex mutex_;

};
}  // namespace common
}  // namespace catena
