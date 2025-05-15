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
 * @brief A collection of mock classes used across the REST tests.
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 25/05/13
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include <IDevice.h>
#include <IParam.h>
#include <ParamDescriptor.h>
#include <Status.h>
#include <Authorization.h>

namespace catena {
namespace common {

// Mock IDevice for testing
class MockDevice : public IDevice {
  public:
    MOCK_METHOD(void, slot, (const uint32_t slot), (override));
    MOCK_METHOD(uint32_t, slot, (), (const, override));
    MOCK_METHOD(std::mutex&, mutex, (), (override));
    MOCK_METHOD(void, detail_level, (const Device_DetailLevel detail_level), (override));
    MOCK_METHOD(Device_DetailLevel, detail_level, (), (const, override));
    MOCK_METHOD(const std::string&, getDefaultScope, (), (const, override));
    MOCK_METHOD(bool, subscriptions, (), (const, override));
    MOCK_METHOD(uint32_t, default_max_length, (), (const, override));
    MOCK_METHOD(uint32_t, default_total_length, (), (const, override));
    MOCK_METHOD(void, set_default_max_length, (const uint32_t default_max_length), (override));
    MOCK_METHOD(void, set_default_total_length, (const uint32_t default_total_length), (override));
    MOCK_METHOD(void, toProto, (Device& dst, Authorizer& authz, bool shallow), (const, override));
    MOCK_METHOD(void, toProto, (LanguagePacks& packs), (const, override));
    MOCK_METHOD(void, toProto, (LanguageList& list), (const, override));
    MOCK_METHOD(exception_with_status, addLanguage, (AddLanguagePayload& language, Authorizer& authz), (override));
    MOCK_METHOD(exception_with_status, getLanguagePack, (const std::string& languageId, ComponentLanguagePack& pack), (const, override));
    class MockDeviceSerializer : public IDeviceSerializer {
      public:
        MOCK_METHOD(bool, hasMore, (), (const, override));
        MOCK_METHOD(DeviceComponent, getNext, (), (override));
    };
    MOCK_METHOD(std::unique_ptr<IDeviceSerializer>, getComponentSerializer, (Authorizer& authz, const std::set<std::string>& subscribedOids, Device_DetailLevel dl, bool shallow), (const, override));
    MOCK_METHOD(void, addItem, (const std::string& key, IParam* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, IConstraint* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, IMenuGroup* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, ILanguagePack* item), (override));
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (const std::string& fqoid, exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (Path& path, exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::vector<std::unique_ptr<IParam>>, getTopLevelParams, (exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::unique_ptr<IParam>, getCommand, (const std::string& fqoid, exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(bool, tryMultiSetValue, (MultiSetValuePayload src, exception_with_status& ans, Authorizer& authz), (override));
    MOCK_METHOD(exception_with_status, commitMultiSetValue, (MultiSetValuePayload src, Authorizer& authz), (override));
    MOCK_METHOD(exception_with_status, setValue, (const std::string& jptr, Value& src, Authorizer& authz), (override));
    MOCK_METHOD(exception_with_status, getValue, (const std::string& jptr, Value& value, Authorizer& authz), (const, override));
    MOCK_METHOD(bool, shouldSendParam, (const IParam& param, bool is_subscribed, Authorizer& authz), (const, override));
};

// Mock IParam for testing
class MockParam : public IParam {
public:
    MockParam() = default;
    virtual ~MockParam() = default;

    // Explicitly declare move semantics
    MockParam(MockParam&&) = default;
    MockParam& operator=(MockParam&&) = default;

    // Explicitly delete copy semantics
    MockParam(const MockParam&) = delete;
    MockParam& operator=(const MockParam&) = delete;

    MOCK_METHOD(std::unique_ptr<IParam>, copy, (), (const, override));
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::Value& dst, Authorizer& authz), (const, override));
    MOCK_METHOD(catena::exception_with_status, fromProto, (const catena::Value& src, Authorizer& authz), (override));
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::Param& param, Authorizer& authz), (const, override));
    MOCK_METHOD(catena::exception_with_status, toProto, (catena::BasicParamInfoResponse& paramInfo, Authorizer& authz), (const, override));
    MOCK_METHOD(ParamType, type, (), (const, override));
    MOCK_METHOD(const std::string&, getOid, (), (const, override));
    MOCK_METHOD(void, setOid, (const std::string& oid), (override));
    MOCK_METHOD(bool, readOnly, (), (const, override));
    MOCK_METHOD(void, readOnly, (bool flag), (override));
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (Path& oid, Authorizer& authz, catena::exception_with_status& status), (override));
    MOCK_METHOD(uint32_t, size, (), (const, override));
    MOCK_METHOD(std::unique_ptr<IParam>, addBack, (Authorizer& authz, catena::exception_with_status& status), (override));
    MOCK_METHOD(catena::exception_with_status, popBack, (Authorizer& authz), (override));
    MOCK_METHOD(const IConstraint*, getConstraint, (), (const, override));
    MOCK_METHOD(const std::string&, getScope, (), (const, override));
    MOCK_METHOD(void, defineCommand, (std::function<catena::CommandResponse(catena::Value)> command), (override));
    MOCK_METHOD(catena::CommandResponse, executeCommand, (const catena::Value& value), (const, override));
    MOCK_METHOD(const ParamDescriptor&, getDescriptor, (), (const, override));
    MOCK_METHOD(bool, isArrayType, (), (const, override));
    MOCK_METHOD(bool, validateSetValue, (const catena::Value& value, Path::Index index, Authorizer& authz, catena::exception_with_status& ans), (override));
    MOCK_METHOD(void, resetValidate, (), (override));
};

/**
 * @brief Mock implementation of ParamDescriptor for testing
 */
class MockParamDescriptor : public ParamDescriptor {
public:
    MockParamDescriptor() : ParamDescriptor(
        catena::ParamType::STRING,        // type
        {},                               // oid_aliases
        {},                               // name
        "",                               // widget
        "",                               // scope
        false,                            // read_only
        "",                               // oid - empty by default
        "",                               // template_oid
        nullptr,                          // constraint
        false,                            // isCommand
        *static_cast<IDevice*>(nullptr),  // device - will be set in constructor
        0,                                // max_length
        0,                                // total_length
        false,                            // minimal_set
        nullptr                           // parent
    ) {}

    MOCK_METHOD((const std::string&), getOid, (), (const));
    MOCK_METHOD(bool, readOnly, (), (const));
    MOCK_METHOD((const std::string&), getScope, (), (const));
    MOCK_METHOD(bool, minimalSet, (), (const));
    MOCK_METHOD((const std::unordered_map<std::string, ParamDescriptor*>&), getAllSubParams, (), (const));
};

} // namespace common
} // namespace catena
