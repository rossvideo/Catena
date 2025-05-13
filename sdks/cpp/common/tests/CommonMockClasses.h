#pragma once

#include <IDevice.h>
#include <IParam.h>
#include <Status.h>
#include <Authorization.h>
#include <gmock/gmock.h>

using namespace catena;
using namespace catena::common;

// Mock IDevice for testing
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
