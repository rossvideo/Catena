/**
* @file Connect_tests.cpp
* @brief Testing for Connect.cpp
* @author nathan.rochon@rossvideo.com
* @date 2025-03-26
* @copyright Copyright © 2025 Ross Video Ltd
*/
//standard includes for testing
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ServiceImpl.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
// the class we are testing
#include <Connect.h>

//nice use of the namespace for testing
using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;




class MockDevice : public Device {
    public:
        MOCK_METHOD(void, slot, (const uint32_t slot), ());
        MOCK_METHOD(uint32_t, slot, (), (const));
        MOCK_METHOD(void, detail_level, (const Device::DetailLevel_e detail_level), ());
        MOCK_METHOD(Device::DetailLevel_e, detail_level, (), (const));
        MOCK_METHOD(const std::string&, getDefaultScope, (), (const));
        MOCK_METHOD(bool, subscriptions, (), (const));
        MOCK_METHOD(uint32_t, default_max_length, (), (const));
        MOCK_METHOD(uint32_t, default_total_length, (), (const));
        MOCK_METHOD(void, set_default_max_length, (const uint32_t default_max_length), ());
        MOCK_METHOD(void, set_default_total_length, (const uint32_t default_total_length), ());
        MOCK_METHOD(void, toProto, (::catena::Device& dst, catena::common::Authorizer& authz, bool shallow), (const));
        MOCK_METHOD(void, toProto, (::catena::LanguagePacks& packs), (const));
        MOCK_METHOD(void, toProto, (::catena::LanguageList& list), (const));
        MOCK_METHOD(catena::exception_with_status, addLanguage, (catena::AddLanguagePayload& language, catena::common::Authorizer& authz), ());
        MOCK_METHOD(catena::exception_with_status, getLanguagePack, (const std::string& languageId, Device::ComponentLanguagePack& pack), (const));
        MOCK_METHOD(Device::DeviceSerializer, getComponentSerializer, (catena::common::Authorizer& authz, bool shallow), (const));
        MOCK_METHOD(Device::DeviceSerializer, getComponentSerializer, (catena::common::Authorizer& authz, const std::vector<std::string>& subscribed_oids, bool shallow), (const));
        MOCK_METHOD(bool, tryMultiSetValue, (catena::MultiSetValuePayload src, catena::exception_with_status& ans, catena::common::Authorizer& authz), ());
        MOCK_METHOD(catena::exception_with_status, commitMultiSetValue, (catena::MultiSetValuePayload src, catena::common::Authorizer& authz), ());
        MOCK_METHOD(catena::exception_with_status, setValue, (const std::string& jptr, catena::Value& src, catena::common::Authorizer& authz), ());
        MOCK_METHOD(catena::exception_with_status, getValue, (const std::string& jptr, catena::Value& value, catena::common::Authorizer& authz), (const));
        MOCK_METHOD(bool, shouldSendParam, (const IParam& param, bool is_subscribed, catena::common::Authorizer& authz), (const));
        MOCK_METHOD((vdk::signal<void(const std::string&, const IParam*, const int32_t)>&), valueSetByServer, (), ());
        MOCK_METHOD((vdk::signal<void(const std::string&, const IParam*, const int32_t)>&), valueSetByClient, (), ());
        MOCK_METHOD((vdk::signal<const Device::ComponentLanguagePack&>&), languageAddedPushUpdate, (), ());

};

class MockSubscriptionManager : public catena::grpc::SubscriptionManager {
    public:
        MOCK_METHOD(bool, addSubscription, (const std::string& oid, catena::common::Device& dm), ());
        MOCK_METHOD(bool, removeSubscription, (const std::string& oid), ());
        MOCK_METHOD(const std::vector<std::string>&, getAllSubscribedOids, (catena::common::Device& dm), ());
        MOCK_METHOD(const std::set<std::string>&, getUniqueSubscriptions, (), (const));
        MOCK_METHOD(const std::set<std::string>&, getWildcardSubscriptions, (), (const));
        MOCK_METHOD(bool, isWildcardMock, (const std::string& oid), (const));

        // Static method redirection to the mock
        static bool isWildcard(const std::string& oid) {
            // Use a static instance of the mock to redirect calls
            static MockSubscriptionManager mockInstance;
            return mockInstance.isWildcardMock(oid);
        }
};
class MockCatenaServiceImpl : public CatenaServiceImpl {
    public:
        // Mock constructor
        MockCatenaServiceImpl(ServerCompletionQueue* cq, MockDevice& dm, std::string& EOPath, bool authz)
            : CatenaServiceImpl(cq, dm, EOPath, authz), cq_(cq), dm_(dm), EOPath_(EOPath), authorizationEnabled_(authz) {}

        MOCK_METHOD(void, init, ());
        MOCK_METHOD(void, processEvents, ());
        MOCK_METHOD(void, shutdownServer, ());
        MOCK_METHOD(void, registerItem, (CallData* cd));
        MOCK_METHOD(void, deregisterItem, (CallData* cd));
        MOCK_METHOD(bool, authorizationEnabled, (), (const));
        // Mock CallData class
        class MockCallData {
            public:
                virtual void proceed(MockCatenaServiceImpl* service, bool ok) = 0;
                virtual ~MockCallData() = default;
        };
    
        // Mocked CallData subclasses
        class MockCallDataImpl : public MockCallData {
            public:
                MOCK_METHOD(void, proceed, (MockCatenaServiceImpl* service, bool ok), (override));
        };
        class MockGetPopulatedSlots : public MockCallData {};
        class MockGetValue : public MockCallData {};
        class MockSetValue : public MockCallData {};
        class MockMultiSetValue : public MockCallData {};
        class MockConnect : public MockCallData {};
        class MockDeviceRequest : public MockCallData {};
        class MockExternalObjectRequest : public MockCallData {};
        class MockBasicParamInfoRequest : public MockCallData {};
        class MockGetParam : public MockCallData {};
        class MockListLanguages : public MockCallData {};
        class MockLanguagePackRequest : public MockCallData {};
        class MockExecuteCommand : public MockCallData {};
        class MockAddLanguage : public MockCallData {};
        class MockUpdateSubscriptions : public MockCallData {};
    
    private:
        ServerCompletionQueue* cq_;
        MockDevice& dm_;
        std::string& EOPath_;
        bool authorizationEnabled_;
        std::mutex registryMutex_;
        std::vector<std::unique_ptr<MockCallData>> registry_;
};

TEST(ConnectTests, Connect_Constructor) {
    
    std::string EOPath = "/test/path";
    bool authEnabled = true;

    grpc::ServerBuilder builder;

    bool ok = true;

    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    cq = builder.AddCompletionQueue();

    MockDevice mockDevice;
    MockCatenaServiceImpl* service = nullptr;

    service = new MockCatenaServiceImpl(cq.get(), mockDevice, EOPath, authEnabled);
    ASSERT_NE(service, nullptr);
    builder.RegisterService(service);

    MockSubscriptionManager subscription_manager;


    ON_CALL(*service, authorizationEnabled()).WillByDefault(Return(true));
    ON_CALL(subscription_manager, addSubscription(_, _)).WillByDefault(Return(true));


    EXPECT_CALL(*service, registerItem).Times(1);
    EXPECT_CALL(*service, authorizationEnabled()).WillOnce(Return(true));
    EXPECT_CALL(mockDevice, slot()).WillOnce(Return(42));
    EXPECT_CALL(subscription_manager, getAllSubscribedOids)
        .WillOnce(Invoke([](catena::common::Device&) -> const std::vector<std::string>& {
            static const std::vector<std::string> empty_vector;
            return empty_vector;
        }));


    CatenaServiceImpl::Connect cc(service, mockDevice, ok, subscription_manager);
    std::cerr << "Connect_Constructor:end" << std::endl;
    // Check that the object was created
    ASSERT_TRUE(&cc);
}