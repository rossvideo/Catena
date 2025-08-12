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
 * @file IDevice.h
 * @brief Interface for the Device class.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright (c) 2025 Ross Video
 */

// common
#include <Authorizer.h>
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

/**
 * @brief Interface class for Device.
 */
class IDevice {
  public:
    /**
     * @brief convenience type aliases to types of objects contained in the
     * device
     */
    using DetailLevel_e = st2138::Device_DetailLevel;
    
    /**
     * @brief Constructs a new Device object.
     */
    IDevice() = default;
    /**
     * @brief Destroys the Device object.
     */
    virtual ~IDevice() = default;

    /**
     * @brief Set the slot number of the device.
     * @param slot The device's new slot number.
     */
    virtual inline void slot(const uint32_t slot) = 0;

    /**
     * @brief Get the slot number of the device
     * @return The device's slot number.
     */
    virtual inline uint32_t slot() const = 0;

    /**
     * @brief Gets the device's mutex.
     * @return The device's mutex.
     */
    virtual inline std::mutex& mutex() = 0;

    /**
     * @brief Sets the default detail level of the device.
     * @param detail_level the new default detail level of the device.
     */
    virtual inline void detail_level(const DetailLevel_e detail_level) = 0;

    /**
     * @brief Get the default detail level of the device.
     * @return The device's default detail level.
     */
    virtual inline DetailLevel_e detail_level() const = 0;
    /**
     * @brief Gets the device's default scope.
     * @return The device's default scope.
     */
    virtual inline const std::string& getDefaultScope() const = 0;

    /**
     * @brief Check if subscriptions are enabled for this device.
     * @return True if subscriptions are enabled, False otherwise.
     */
    virtual inline bool subscriptions() const = 0;

    /**
     * @brief Gets the default max length for this device's array params.
     * @return The default max length for this device's array params.
     */
    virtual inline uint32_t default_max_length() const = 0;
    /**
     * @brief Gets the default total length for this device's string array
     * params.
     * @return The default total length for this device's string array params.
     */
    virtual inline uint32_t default_total_length() const = 0;

    /**
     * @brief Sets the default max length for this device's array params.
     * @param default_max_length The device's new default max length.
     */
    virtual void set_default_max_length(const uint32_t default_max_length) = 0;
    /**
     * @brief Sets the default total length for this device's array params.
     * @param default_total_length The device's new default total length.
     */
    virtual void set_default_total_length(const uint32_t default_total_length) = 0;

    /**
     * @brief Create a protobuf representation of the device.
     * @param dst the protobuf representation of the device.
     * @param shallow if true, only the top-level info is copied, params,
     * commands etc are not copied. Design intent is to permit large models to
     * stream their parameters instead of sending a huge device model in one
     * big lump.
     */
    virtual void toProto(::st2138::Device& dst, const IAuthorizer& authz, bool shallow = true) const = 0;

    /**
     * @brief Creates a protobuf representation of the device's language packs.
     * @param packs the protobuf representation of the language packs.
     */
    virtual void toProto(::st2138::LanguagePacks& packs) const = 0;

    /**
     * @brief Populates a protobuf LanguageList with the language IDs of the
     * device's supported languages.
     * @param list The protobuf language list object.
     */
    virtual void toProto(::st2138::LanguageList& list) const = 0;

    /**
     * @brief Returns true if device supports the specified language.
     * @param languageId The language id to check for (e.g. "en").
     * @return True if the device supports specified language pack.
     */
    virtual bool hasLanguage(const std::string& languageId) const = 0;

    /**
     * @brief Adds a language pack to the device. Requires client to have
     * admin:w scope.
     * @param language The language to add to the device.
     * @param authz The authorizer object containing client's scopes.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     */
    virtual catena::exception_with_status addLanguage(st2138::AddLanguagePayload& language, const IAuthorizer& authz = Authorizer::kAuthzDisabled) = 0;

    /**
     * @brief Removes a language pack from the device. Requires client to have
     * admin:w scope. Fails if the language pack was shipped with the device.
     * @param language The language to remove from the device.
     * @param authz The authorizer object containing client's scopes.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     */
    virtual catena::exception_with_status removeLanguage(const std::string& languageId, const IAuthorizer& authz = Authorizer::kAuthzDisabled) = 0;

    using ComponentLanguagePack = st2138::DeviceComponent_ComponentLanguagePack;
    /**
     * @brief Finds and returns a language pack based on languageId.
     * @param languageId The language id of the language pack to get
     * (e.g. "en").
     * @param pack Output var containing the found LanguagePack.
     * @return exception_with_status containing the status of the operation.
     */
    virtual catena::exception_with_status getLanguagePack(const std::string& languageId, ComponentLanguagePack& pack) const = 0;

    /**
     * @brief DeviceSerializer is a coroutine that serializes the device into a
     * stream of DeviceComponents.
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
         * serialized by the coroutine.
         */
        virtual inline bool hasMore() const = 0;

        /**
         * @brief Gets the next DeviceComponent to be serialized.
         * @return The next DeviceComponent.
         * 
         * If the coroutine is done and there are no more components to
         * serialize then an empty DeviceComponent is returned.
         */
        virtual st2138::DeviceComponent getNext() = 0;
    };

    /**
     * @brief Gets a serializer for the device.
     * 
     * @param authz The authorizer object containing the scopes of the client.
     * @param subscribedOids The oids of the subscribed parameters.
     * @param dl The detail level to retrieve information in.
     * @param shallow If true, the device will be returned in parts, otherwise
     * the whole device will be returned in one message.
     * @return A DeviceSerializer object.
     */
    virtual std::unique_ptr<IDeviceSerializer> getComponentSerializer(const IAuthorizer& authz, const std::set<std::string>& subscribedOids, st2138::Device_DetailLevel dl, bool shallow = false) const = 0;

    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for parameters and commands.
     * 
     * @param key The item's unique key.
     * @param item The item to be added.
     */
    virtual void addItem(const std::string& key, IParam* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for constraints.
     * 
     * @param key The item's unique key.
     * @param item The item to be added.
     */
    virtual void addItem(const std::string& key, IConstraint* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device.
     * Overload for menu groups.
     * 
     * @param key The item's unique key
     * @param item The item to be added
     */
    virtual void addItem(const std::string& key, IMenuGroup* item) = 0;
    /**
     * @brief add an item to one of the collections owned by the device
     * Overload for language packs.
     * 
     * @param key The item's unique key
     * @param item The item to be added
     */
    virtual void addItem(const std::string& key, ILanguagePack* item) = 0;

    /**
     * @brief Get a parameter by oid with authorization.
     * @param fqoid The fully qualified oid of the parameter.
     * @param status will contain an error message if the parameter does not
     * exist.
     * @param authz The IAuthorizer to test read permission with.
     * @return a unique pointer to the parameter, or nullptr if it does not
     * exist.
     */
    virtual std::unique_ptr<IParam> getParam(const std::string& fqoid, catena::exception_with_status& status, const IAuthorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief Gets a parameter by oid with authorization.
     * @param path The full path to the parameter.
     * @param status Will contain an error message if the parameter does not
     * exist.
     * @param authz The authorizer object to test read permission with.
     * @return a unique pointer to the parameter, or nullptr if it does not
     * exist.
     */
    virtual std::unique_ptr<IParam> getParam(catena::common::Path& path, catena::exception_with_status& status, const IAuthorizer& authz = Authorizer::kAuthzDisabled) const = 0;
    
    /**
     * @brief Gets all top level parameters.
     * @param status The status of the operation.
     * @param authz The authorizer object.
     * @return A vector of unique pointers to the top level parameters.
     */
    virtual std::vector<std::unique_ptr<IParam>> getTopLevelParams(catena::exception_with_status& status, const IAuthorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief Gets a command by oid.
     * @param fqoid The fully qualified oid of the command.
     * @param authz The authorizer object.
     * @param status Will contain an error message if the command does not
     * exist.
     * @return A unique pointer to the command, or nullptr if it does not exist.
     */
    virtual std::unique_ptr<IParam> getCommand(const std::string& fqoid, catena::exception_with_status& status, const IAuthorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief Validates each payload in a multiSetValuePayload without commit
     * changes to param value_s.
     * @param src The MultiSetValuePayload to validate.
     * @param ans The exception_with_status to return.
     * @param authz The IAuthorizer to test with.
     * @returns True if the call is valid.
     */
    virtual bool tryMultiSetValue (st2138::MultiSetValuePayload src, catena::exception_with_status& ans, const IAuthorizer& authz = Authorizer::kAuthzDisabled) = 0;
    
    /**
     * @brief Sets the values of a device's parameter's using a
     * MultiSetValuePayload.
     * This function assumes that tryMultiSetValue is called at some point
     * beforehand to validate the call.
     * @param src The MultiSetValuePayload to update the device with.
     * @param authz The Authroizer with the client's scopes.
     * @returns An exception_with_status with status set OK if successful.
     */
    virtual catena::exception_with_status commitMultiSetValue (st2138::MultiSetValuePayload src, const IAuthorizer& authz) = 0;

    /**
     * @brief Deserialize a protobuf value object into the parameter value
     * pointed to by jptr.
     * @param jptr Json pointer to the part of the device model to update.
     * @param src The value to update the parameter with.
     * @param authz The IAuthorizer to test with.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     */
    virtual catena::exception_with_status setValue (const std::string& jptr, st2138::Value& src, const IAuthorizer& authz = Authorizer::kAuthzDisabled) = 0;

    /**
     * @brief Serialize the parameter value to protobuf.
     * @param jptr Json pointer to the part of the device model to serialize.
     * @param dst the protobuf value to serialize to.
     * @param authz The IAuthorizer to test read permission with.
     * @return An exception_with_status with status set OK if successful,
     * otherwise an error.
     */
    virtual catena::exception_with_status getValue (const std::string& jptr, st2138::Value& value, const IAuthorizer& authz = Authorizer::kAuthzDisabled) const = 0;

    /**
     * @brief Check if a parameter should be sent based on detail level and
     * authorization.
     * @param param The parameter to check.
     * @param is_subscribed True if the parameter is subscribed, false
     * otherwise.
     * @param authz The authorizer object.
     * @return True if the parameter should be sent, false otherwise.
     */
    virtual bool shouldSendParam(const IParam& param, bool is_subscribed, const IAuthorizer& authz) const = 0;

    /**
     * @brief Get the signal emitted when a value is set by the client.
     * @return The signal.
     */
    virtual vdk::signal<void(const std::string&, const IParam*)>& getValueSetByClient() = 0;

    /**
     * @brief Get the signal emitted when a language pack is added to the
     * device.
     * @return The signal.
     */
    virtual vdk::signal<void(const ILanguagePack*)>& getLanguageAddedPushUpdate() = 0;

    /**
     * @brief Get the signal emitted when a value is set by the server, or
     * business logic.
     * @return The signal.
     */
    virtual vdk::signal<void(const std::string&, const IParam*)>& getValueSetByServer() = 0;

    /**
     * @brief Get the asset request signal.
     * @return The signal.
     */
    virtual vdk::signal<void(const std::string&, const IAuthorizer*)>& getDownloadAssetRequest() = 0;

    /**
      * @brief Get the upload asset request signal.
      * @return The signal.
      */
    virtual vdk::signal<void(const std::string&, const IAuthorizer*)>& getUploadAssetRequest() = 0;
    /**
     * @brief Get the delete asset request signal.
     * @return The signal.
     */
    virtual vdk::signal<void(const std::string&, const IAuthorizer*)>& getDeleteAssetRequest() = 0;
};

/**
 * @brief Type alias for a map of slot numbers to their corresponding device.
 * 
 * Designed for use in the catena APIs.
 */
using SlotMap = std::unordered_map<uint32_t, IDevice*>;
/**
 * @brief A map for storing signals and the slot of the device they belong to.
 * 
 * Designed for use in the catena APIs connect handlers for signal
 * disconnection.
 */
using SignalMap = std::unordered_map<uint32_t, uint32_t>;

}  // namespace common
}  // namespace catena
