/**
* @file UpdateSubscriptions_tests.cpp
* @brief Testing for UpdateSubscriptions.cpp
* @author nathan.rochon@rossvideo.com
* @date 2025-03-26
* @copyright Copyright © 2025 Ross Video Ltd
*/


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "UpdateSubscriptions.h"
#include "SubscriptionManager.h"
#include "ServiceCredentials.h"
#include "ServiceImpl.h"
#include "Device.h"
// #include "../src/UpdateSubscriptions.cpp"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;
///https://github.com/ElectricRCAircraftGuy/eRCaGuy_dotfiles/blob/master/googletest/insights.md

// Mock classes
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
// Mock class for grpc::ServerAsyncWriter
class MockServerAsyncWriter {
public:
    MOCK_METHOD(void, Write, (const catena::DeviceComponent_ComponentParam& msg, void* tag), ());
};
class MockCatenaServiceImpl : public CatenaServiceImpl {
    public:
        // Mock constructor
        MockCatenaServiceImpl(grpc::ServerCompletionQueue* cq, Device& dm, std::string& EOPath, bool authorizationEnabled)
            : CatenaServiceImpl(cq, dm, EOPath, authorizationEnabled) {}

        MOCK_METHOD(void, registerItem, (const std::string& oid), ());
        MOCK_METHOD(bool, authorizationEnabled, (), (const));
        MOCK_METHOD(void*, getCompletionQueue, (), (const));
        MOCK_METHOD(void*, getServer, (), (const));
        MOCK_METHOD(MockServerAsyncWriter*, getWriter, (), (const));
};
//Fixture
class UpdateSubscriptionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        eoPath = "/test/path";
        bool authEnabled = false;

        grpc::ServerBuilder builder;

        // Create a completion queue
        cq = builder.AddCompletionQueue();
 
        // Create the service instance
        service = new MockCatenaServiceImpl(cq.get(), mockDevice, eoPath, authEnabled);

        // Register the service with the server
        builder.RegisterService(service);

        // Build and start the server
        server = builder.BuildAndStart();

        ASSERT_TRUE(server != nullptr) << "Failed to start gRPC server.";
    }

    void TearDown() override {
        //shutdown the server
        if (server){
            server->Shutdown();
            server->Wait(); 
        }
        
        if (cq){
            cq->Shutdown();
            void* ignored_tag;
            bool ignored_ok;
            while (cq->Next(&ignored_tag, &ignored_ok)) { }
        }
        if (service){
            delete service;
            service = nullptr;
        }

        
    }
    std::string eoPath;
    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    std::unique_ptr<grpc::Server> server;
    MockDevice mockDevice;
    MockSubscriptionManager mockSubscriptionManager;
    MockCatenaServiceImpl* service = nullptr;
};

TEST_F(UpdateSubscriptionsTest, UpdateSubscriptions_Constructor) {
    // Create a new UpdateSubscriptions object
    bool ok = true;

    auto us = new CatenaServiceImpl::UpdateSubscriptions(service, mockDevice, mockSubscriptionManager, ok);

    ASSERT_TRUE(ok);
    
    // Verify that the object is created
    SUCCEED();
 
}

// TEST_F(UpdateSubscriptionsTest, UpdateSubscriptions_Proceed) {
//     // Create a new UpdateSubscriptions object
//     bool ok = true;

//     auto us = new CatenaServiceImpl::UpdateSubscriptions(service, mockDevice, mockSubscriptionManager, ok);

//     // Call the proceed method
//     // null service
//     us->proceed(nullptr, ok);

//     us->proceed(service, ok);
//     // Verify that the object is created
//     SUCCEED();
// }
