#pragma once

/*
 * Copyright 2025 Ross Video Ltd
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
 * @file IDevice.h
 * @brief Interface for device
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright (c) 2025 Ross Video
 */

// common
#include <Authorization.h>
#include <Path.h>
#include <IParam.h>
#include <ILanguagePack.h>
#include <Status.h>
#include <vdk/signals.h>

// protobuf interface
#include <interface/device.pb.h>

#include <string>
#include <vector>
#include <coroutine>
#include <mutex>

namespace catena {
namespace common {

using SlotMap = std::unordered_map<uint32_t, IDevice*>;

/**
 * @brief Interface class for Device.
 */
class IDevice {
  public:
    /**
     * @brief convenience type aliases to types of objects contained in the
     * device
     */
    using DetailLevel_e = catena::Device_DetailLevel;
    
    /**
     * @brief Construct a new Device object
     */
    IDevice() = default;
    /**
     * @brief Destroy the Device object
     */
    virtual ~IDevice() = default;

    /**
     * @brief set the slot number of the device
     * @param slot the slot number of the device
     */
    virtual inline void slot(const uint32_t slot) = 0;

    /**
     * @brief get the slot number of the device
     * @return slot number
     */
    virtual inline uint32_t slot() const = 0;

    /**
     * @brief Returns the device's mutex.
     */
    virtual inline std::mutex& mutex() = 0;

    /**
     * @brief set the detail level of the device
     * @param detail_level the detail level of the device
     */
    virtual inline void detail_level(const DetailLevel_e detail_level) = 0;

    /**
     * @brief get the detail level of the device
     * @return DetailLevel_
     */
    virtual inline DetailLevel_e detail_level() const = 0;
    /**
     * @brief Returns the device's default scope.
     */
    virtual inline const std::string& getDefaultScope() const = 0;

    /**
     * @brief Check if subscriptions are enabled for this device
     * @return true if subscriptions are enabled, false otherwise
     */
    virtual inline bool subscriptions() const = 0;

    /**
     * @return The default max length for this device's array params.
     */
    virtual inline uint32_t default_max_length() const = 0;
    /**
     * @return The default total length for this device's string array params.
     */
    virtual inline uint32_t default_total_length() const = 0;

    /**
     * @brief Sets the default_max_length_ for this device's array params.
     * If default_max_length <= 0, then it reverts default_max_length_ to
     * kDefaultMaxArrayLength.
     * @param default_max_length The value to set default_max_length_ to.
     */
    virtual void set_default_max_length(const uint32_t default_max_length) = 0;
    /**
     * @brief Sets the default_total_length_ for this device's array params.
     * If default_total_length <= 0, then it reverts default_total_length_ to
     * kDefaultMaxArrayLength.
     * @param default_total_length The value to set default_total_length_ to.
     */
    virtual void set_default_total_length(const uint32_t default_total_length) = 0;

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
    virtual void toProto(::catena::Device& dst, Authorizer& authz, bool shallow = true) const = 0;

    /**
     * @brief Create a protobuf representation of the language packs.
     * @param packs the protobuf representation of the language packs.
     */
    virtual void toProto(::catena::LanguagePacks& packs) const = 0;

    /**
     * @brief Create a protobuf representation of the language list.
     * @param list the protobuf representation of the language list.
     */
    virtual void toProto(::catena::LanguageList& list) const = 0;

    using ComponentLanguagePack = catena::DeviceComponent_ComponentLanguagePack;

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
    virtual catena::exception_with_status addLanguage (catena::AddLanguagePayload& language, Authorizer& authz = Authorizer::kAuthzDisabled) = 0;

    /**
     * @brief Finds and returns a language pack based on languageId.
     * @param languageId The language id of the language pack e.g. "en"
     * @param pack Output var containing the found LanguagePack.
     * @return exception_with_status containing the status of the operation.
     */
    virtual catena::exception_with_status getLanguagePack(const std::string& languageId, ComponentLanguagePack& pack) const = 0;

    /**
     * @brief DeviceSerializer is a coroutine that serializes the device into a
     * stream of DeviceComponents
     * This struct manages the state and lifetime of the coroutine. It also
     * provides the interface for resuming the coroutine.
     * 
     * Coroutine types are required to contain a promise_type struct that
     * defines some special member functions that the compiler will call
     * when the coroutine is created, destroyed, or resumed. This promise_type
     * struct must be defined in the inherited class.
     */
    class IDeviceSerializer {
      public:
        IDeviceSerializer() = default;

        IDeviceSerializer(const IDeviceSerializer&) = delete;
        IDeviceSerializer& operator=(const IDeviceSerializer&) = delete;
        
        virtual ~IDeviceSerializer() = default;

        /**
         * @brief returns true if there are more DeviceComponents to be
         * serialized
         */
        virtual inline bool hasMore() const = 0;

        /**
         * @brief get the next DeviceComponent to be serialized.
         * @return the next DeviceComponent
         * 
         * If the coroutine is done and there are no more components to
         * serialize then an empty DeviceComponent is returned.
         */
        virtual catena::DeviceComponent getNext() = 0;
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
    virtual std::unique_ptr<IDeviceSerializer> getComponentSerializer(Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow = false) const = 0;

    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for parameters and commands.
     * @param key item's unique key
     * @param item the item to be added
     */
    virtual void addItem(const std::string& key, IParam* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for constraints.
     * @param key item's unique key
     * @param item the item to be added
     */
    virtual void addItem(const std::string& key, IConstraint* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for menu groups.
     * @param key item's unique key
     * @param item the item to be added
     */
    virtual void addItem(const std::string& key, IMenuGroup* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device
     * Overload for language packs.
     * @param key item's unique key
     * @param item the item to be added
     */
    virtual void addItem(const std::string& key, ILanguagePack* item) = 0;

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
    virtual std::unique_ptr<IParam> getParam(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const = 0;

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
    virtual std::unique_ptr<IParam> getParam(catena::common::Path& path, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const = 0;
    
    /**
     * @brief get all top level parameters
     * @param status the status of the operation
     * @param authz the authorizer object
     * @return a vector of unique pointers to the parameters
     */
    virtual std::vector<std::unique_ptr<IParam>> getTopLevelParams(catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief get a command by oid
     * @param fqoid the fully qualified oid of the command
     * @param authz the authorizer object
     * @param status will contain an error message if the command does not
     * exist
     * @return a unique pointer to the command, or nullptr if it does not exist
     * @todo add authorization checking
     */
    virtual std::unique_ptr<IParam> getCommand(const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief Validates each payload in a multiSetValuePayload without commit
     * changes to param value_s.
     * @param src The MultiSetValuePayload to validate.
     * @param ans The exception_with_status to return.
     * @param authz The Authorizer to test with.
     * @returns true if the call is valid.
     */
    virtual bool tryMultiSetValue (catena::MultiSetValuePayload src, catena::exception_with_status& ans, Authorizer& authz = Authorizer::kAuthzDisabled) = 0;
    
    /**
     * @brief Sets the values of a device's parameter's using a
     * MultiSetValuePayload.
     * This function assumes that tryMultiSetValue is called at some point
     * beforehand to validate the call.
     * @param src The MultiSetValuePayload to update the device with.
     * @param authz The Authroizer with the client's scopes.
     * @returns an exception_with_status with status set OK if successful.
     */
    virtual catena::exception_with_status commitMultiSetValue (catena::MultiSetValuePayload src, Authorizer& authz) = 0;

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
    virtual catena::exception_with_status setValue (const std::string& jptr, catena::Value& src, Authorizer& authz = Authorizer::kAuthzDisabled) = 0;

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
    virtual catena::exception_with_status getValue (const std::string& jptr, catena::Value& value, Authorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief check if a parameter should be sent based on detail level and
     * authorization
     * @param param the parameter to check
     * @param is_subscribed true if the parameter is subscribed, false
     * otherwise
     * @param authz the authorizer object
     * @return true if the parameter should be sent, false otherwise
     */
    virtual bool shouldSendParam(const IParam& param, bool is_subscribed, Authorizer& authz) const = 0;

    /**
     * @brief signal emitted when a value is set by the client.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const std::string&, const IParam*)> valueSetByClient;

    /**
     * @brief signal emitted when a language pack is added to the device.
     * Intended recipient is the business logic.
     */
    vdk::signal<void(const ILanguagePack*)> languageAddedPushUpdate;

    /**
     * @brief signal emitted when a value is set by the server, or business
     * logic.
     * Intended recipient is the connection manager.
     */
    vdk::signal<void(const std::string&, const IParam*)> valueSetByServer;
};

}  // namespace common
}  // namespace catena
