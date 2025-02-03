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
#include <Authorization.h>
#include <Path.h>
#include <Enums.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <ILanguagePack.h>
#include <Tags.h>  
#include <Status.h>
#include <IMenu.h>
#include <IMenuGroup.h>

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
 * @brief Implements the Device interface defined in the protobuf
 */
class Device {
  public:
    /**
     * @brief LockGuard is a helper class to lock and unlock the device mutex
     */
    class LockGuard {
      public:
        LockGuard(Device& dm) : dm_(dm) { dm_.mutex_.lock(); }
        ~LockGuard() { dm_.mutex_.unlock(); }

      private:
        Device& dm_;
    };
    friend class LockGuard;

    /**
     * @brief convenience type aliases to types of objects contained in the device
     */
    using DetailLevel_e = catena::Device_DetailLevel;

  public:
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
      default_scope_{default_scope}, multi_set_enabled_{multi_set_enabled}, subscriptions_{subscriptions} {}

    /**
     * @brief Destroy the Device object
     */
    virtual ~Device() = default;

    /**
     * @brief set the slot number of the device
     * @param slot the slot number of the device
     */
    inline void slot(const uint32_t slot) { slot_ = slot; }

    /**
     * @brief get the slot number of the device
     * @return slot number
     */
    inline uint32_t slot() const { return slot_; }

    /**
     * @brief set the detail level of the device
     * @param detail_level the detail level of the device
     */
    inline void detail_level(const DetailLevel_e detail_level) { detail_level_ = detail_level; }

    /**
     * @brief get the detail level of the device
     * @return DetailLevel_e
     */
    inline DetailLevel_e detail_level() const { return detail_level_; }

    inline const std::string& getDefaultScope() const { return default_scope_; }

    /**
     * @brief Create a protobuf representation of the device.
     * @param dst the protobuf representation of the device.
     * @param shallow if true, only the top-level info is copied, params, commands etc 
     * are not copied. Design intent is to permit large models to stream their parameters
     * instead of sending a huge device model in one big lump.
     * 
     * N.B. This method is not thread-safe. It is the caller's responsibility to ensure
     * that the device is not modified while this method is running. This class provides
     * a LockGuard helper class to make this easier.
     */
    void toProto(::catena::Device& dst, Authorizer& authz, bool shallow = true) const;

    /**
     * @brief Create a protobuf representation of the language packs.
     * @param packs the protobuf representation of the language packs.
     */
    void toProto(::catena::LanguagePacks& packs) const;

    /**
     * @brief Create a protobuf representation of the language list.
     * @param list the protobuf representation of the language list.
     */
    void toProto(::catena::LanguageList& list) const;

    using componentLanguagePack = catena::DeviceComponent_ComponentLanguagePack;
    /**
     * @brief Finds and returns a language pack based on languageId.ABORTED
     * @param languageId The language id of the language pack e.g. "en"
     * @param pack Output var containing the found LanguagePack.
     * @return exception_with_status containing the status of the operation.
     */
    catena::exception_with_status getLanguagePack(const std::string& languageId, componentLanguagePack& pack) const;

    /**
     * @brief DeviceSerializer is a coroutine that serializes the device into a stream of DeviceComponents
     * This struct manages the state and lifetime of the coroutine. It also provides the interface for resuming the coroutine.
     */
    class DeviceSerializer {
      public:
        struct promise_type; // forward declaration

      private:
        /**
         * The coroutine handle is a pointer to the coroutine state
         * 
         * The coroutine handle provides the following operations:
         * resume() - resumes the coroutine from where it was suspended
         * destroy() - destroys the coroutine
         * done() - returns true if the coroutine has completed (i.e. co_return has been called)
         */
        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle_;

      public:
        /** 
         * @brief Defines the execution behaviour of the coroutine
         * 
         * Coroutine types are required to contain a promise_type struct that defines some special
         * member functions that the compiler will call when the coroutine is created, destroyed,
         * or resumed.
         */
        struct promise_type {

            /**
             * get_return_object() is called when the coroutine is created (i.e. when getComponentSerializer() 
             * is called). It creates the handle for the coroutine and returns the DeviceSerializer object.
             */
            inline DeviceSerializer get_return_object() { 
              return DeviceSerializer(handle_type::from_promise(*this)); 
            }

            /**
             * initial_suspend() is called when the coroutine is created. It returns a std::suspend_always awaitable
             * so that the coroutine will be immediately suspended. This means the first component will not be created
             * until getNext() is called.
             */
            inline std::suspend_always initial_suspend() { return {}; }

            /** 
             * final_suspend() is called when the coroutine is completed. It returns a std::suspend_always awaitable
             * causing the coroutine to be suspended before it destroys itself. The coroutine will be destroyed when
             * the device serializer destructor is called.
             */
            inline std::suspend_always final_suspend() noexcept { return {}; }

            /**
             * yield_value() is called when the coroutine reaches a co_yield statement. It stores the yielded
             * DeviceComponent then suspends the coroutine.
             */
            inline std::suspend_always yield_value(catena::DeviceComponent& component) { 
              deviceMessage = component;
              return {}; 
            }

            /**
             * return_value() is called when the coroutine reaches a co_return statement. It stores the returned
             * DeviceComponent. The coroutine is then set as done.
             */
            inline void return_value(catena::DeviceComponent component) { this->deviceMessage = component; }

            /**
             * unhandled_exception() is called if an exception is thrown in the coroutine. It stores the exception
             * so that it can be rethrown by the function that resumed the coroutine.
             */
            inline void unhandled_exception() {
              exception_ = std::current_exception();
            }

            /**
             * rethrow_if_exception() is not called by the compiler but instead is called by the function that
             * resumes the coroutine. It rethrows the exception that was caught by unhandled_exception().
             */
            inline void rethrow_if_exception() {
              if (exception_) std::rethrow_exception(exception_);
            }

            catena::DeviceComponent deviceMessage{};
            std::exception_ptr exception_; 
        };

        DeviceSerializer(handle_type h) : handle_(h) {}

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
         * @brief returns true if there are more DeviceComponents to be serialized
         */
        inline bool hasMore() const { return handle_ && !handle_.done(); }

        /**
         * @brief get the next DeviceComponent to be serialized.
         * @return the next DeviceComponent
         * 
         * If the coroutine is done and there are no more components to serialize then
         * an empty DeviceComponent is returned.
         */
        catena::DeviceComponent getNext();
    };

    /**
     * @brief get a serializer for the device
     * @param clientScopes the scopes of the client
     * @param shallow if true, the device will be returned in parts, otherwise the whole device will be returned in one message
     * @return a DeviceSerializer object
     */
    DeviceSerializer getComponentSerializer(Authorizer& authz, bool shallow = false) const;

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    template <typename TAG>
    void addItem(const std::string& key, typename TAG::type* item) {
        if constexpr (std::is_same_v<TAG, common::ParamTag>) {
            params_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::ConstraintTag>) {
            constraints_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::MenuGroupTag>) {
            menu_groups_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::CommandTag>) {
            commands_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, common::LanguagePackTag>) {
            language_packs_[key] = item;
        }
    }

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
     * @param authz the authorizer object
     * @param status will contain an error message if the parameter does not exist
     * @return a unique pointer to the parameter, or nullptr if it does not exist
     * 
     * gets a parameter if it exists and the client is authorized to read it.
     */
    std::unique_ptr<IParam> getParam(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const;

    /**
     * @brief get a command by oid
     * @param fqoid the fully qualified oid of the command
     * @param authz the authorizer object
     * @param status will contain an error message if the command does not exist
     * @return a unique pointer to the command, or nullptr if it does not exist
     * @todo add authorization checking
     */
    std::unique_ptr<IParam> getCommand(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const;

    /**
     * @brief Determines if it's possible to set the specified value given the
     * current authorization.
     * @param jptr json pointer to the part of the device model to update.
     * @param authz the Authorizer to test with.
     * @return an exception_with_status with status set OK if possible,
     * otherwise an error.
     * Intention is for MultiSetValue RPCs / API calls to be verified in their
     * entirely before setting any values.
     */
    catena::exception_with_status setValueTry (const std::string& jptr, Authorizer& authz = Authorizer::kAuthzDisabled);

    /**
     * @brief deserialize a protobuf value object into the parameter value
     * pointed to by jptr.
     * @param jptr json pointer to the part of the device model to update.
     * @param src the value to update the parameter with.
     * @todo consider using move semantics on the value parameter to emphasize new ownership.
     * @return an exception_with_status with status set OK if successful, otherwise an error.
     * Intention is to for SetValue RPCs / API calls to be serviced by this method.
     * @note on success, this method will emit the valueSetByClient signal.
     */
    catena::exception_with_status setValue (const std::string& jptr, catena::Value& src, Authorizer& authz = Authorizer::kAuthzDisabled);

    /**
     * @brief serialize the parameter value to protobuf
     * @param jptr, json pointer to the part of the device model to serialize.
     * @param dst, the protobuf value to serialize to.
     * @return an exception_with_status with status set OK if successful, otherwise an error.
     * Intention is to for GetValue RPCs / API calls to be serviced by this method.
     */
    catena::exception_with_status getValue (const std::string& jptr, catena::Value& value, Authorizer& authz = Authorizer::kAuthzDisabled) const;

  public:
    /**
     * @brief signal emitted when a value is set by the client.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByClient;
    
    /**
     * @brief signal emitted when a value is set by the server, or business logic.
     * Intended recipient is the connection manager.
     */
    vdk::signal<void(const std::string&, const IParam*, const int32_t)> valueSetByServer;

  private:
    uint32_t slot_;
    Device_DetailLevel detail_level_;
    std::unordered_map<std::string, catena::common::IConstraint*> constraints_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, common::IMenuGroup*> menu_groups_;
    std::unordered_map<std::string, IParam*> commands_;
    std::unordered_map<std::string, common::ILanguagePack*> language_packs_;
    std::vector<std::string> access_scopes_;
    std::string default_scope_;
    bool multi_set_enabled_;
    bool subscriptions_;
    
    mutable std::mutex mutex_;
};
}  // namespace common
}  // namespace catena
