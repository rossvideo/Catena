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
 * @brief A colleciton of mock classes used across the REST API tests.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/12
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include "interface/ISocketReader.h"
#include <IDevice.h>

using namespace catena::REST;
using namespace catena::common;

// Mocking the SocketReader interface
class MockSocketReader : public ISocketReader {
  public:
    MOCK_METHOD(void, read, (tcp::socket& socket, bool authz), (override));
    MOCK_METHOD(const std::string&, method, (), (const, override));
    MOCK_METHOD(const std::string&, endpoint, (), (const, override));
    MOCK_METHOD(uint32_t, slot, (), (const, override));
    MOCK_METHOD(bool, hasField, (const std::string& key), (const, override));
    MOCK_METHOD(const std::string&, fields, (const std::string& key), (const, override));
    MOCK_METHOD(const std::string&, jwsToken, (), (const, override));
    MOCK_METHOD(const std::string&, origin, (), (const, override));
    MOCK_METHOD(const std::string&, language, (), (const, override));
    MOCK_METHOD(catena::Device_DetailLevel, detailLevel, (), (const, override));
    MOCK_METHOD(const std::string&, jsonBody, (), (const, override));
    MOCK_METHOD(ISubscriptionManager&, getSubscriptionManager, (), (override));
    MOCK_METHOD(bool, authorizationEnabled, (), (const, override));
};
  
// Mocking the Device interface
class MockDevice : public IDevice {
  public:
    MOCK_METHOD(void, slot, (const uint32_t slot), (override));
    MOCK_METHOD(uint32_t, slot, (), (const, override));
    MOCK_METHOD(std::mutex&, mutex, (), (override));
    MOCK_METHOD(void, detail_level, (const catena::Device_DetailLevel detail_level), (override));
    MOCK_METHOD(catena::Device_DetailLevel, detail_level, (), (const, override));
    MOCK_METHOD(const std::string&, getDefaultScope, (), (const, override));
    MOCK_METHOD(bool, subscriptions, (), (const, override));
    MOCK_METHOD(uint32_t, default_max_length, (), (const, override));
    MOCK_METHOD(uint32_t, default_total_length, (), (const, override));
    MOCK_METHOD(void, set_default_max_length, (const uint32_t default_max_length), (override));
    MOCK_METHOD(void, set_default_total_length, (const uint32_t default_total_length), (override));
    MOCK_METHOD(void, toProto, (catena::Device& dst, Authorizer& authz, bool shallow), (const, override));
    MOCK_METHOD(void, toProto, (catena::LanguagePacks& packs), (const, override));
    MOCK_METHOD(void, toProto, (catena::LanguageList& list), (const, override));
    MOCK_METHOD(catena::exception_with_status, addLanguage, (catena::AddLanguagePayload& language, Authorizer& authz), (override));
    MOCK_METHOD(catena::exception_with_status, getLanguagePack, (const std::string& languageId, ComponentLanguagePack& pack), (const, override));
    class MockDeviceSerializer : public IDeviceSerializer {
      public:
        MOCK_METHOD(bool, hasMore, (), (const, override));
        MOCK_METHOD(catena::DeviceComponent, getNext, (), (override));
    };
    MOCK_METHOD(std::unique_ptr<IDeviceSerializer>, getComponentSerializer, (Authorizer& authz, const std::set<std::string>& subscribedOids, catena::Device_DetailLevel dl, bool shallow), (const, override));
    MOCK_METHOD(void, addItem, (const std::string& key, IParam* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, IConstraint* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, IMenuGroup* item), (override));
    MOCK_METHOD(void, addItem, (const std::string& key, ILanguagePack* item), (override));
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::unique_ptr<IParam>, getParam, (Path& path, catena::exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::vector<std::unique_ptr<IParam>>, getTopLevelParams, (catena::exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(std::unique_ptr<IParam>, getCommand, (const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz), (const, override));
    MOCK_METHOD(bool, tryMultiSetValue, (catena::MultiSetValuePayload src, catena::exception_with_status& ans, Authorizer& authz), (override));
    MOCK_METHOD(catena::exception_with_status, commitMultiSetValue, (catena::MultiSetValuePayload src, Authorizer& authz), (override));
    MOCK_METHOD(catena::exception_with_status, setValue, (const std::string& jptr, catena::Value& src, Authorizer& authz), (override));
    MOCK_METHOD(catena::exception_with_status, getValue, (const std::string& jptr, catena::Value& value, Authorizer& authz), (const, override));
    MOCK_METHOD(bool, shouldSendParam, (const IParam& param, bool is_subscribed, Authorizer& authz), (const, override));
};