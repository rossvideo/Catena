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
#include "Device.h"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

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
class MockCatenaServiceImpl : public catena::CatenaService::AsyncService {
public:
    MOCK_METHOD(void, RequestUpdateSubscriptions,
                (grpc::ServerContext* context,
                    catena::UpdateSubscriptionsPayload* request,
                    grpc::ServerAsyncWriter<catena::DeviceComponent_ComponentParam>* writer,
                    grpc::ServerCompletionQueue* cq,
                    grpc::ServerCompletionQueue* notification_cq,
                    void* tag), ());
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
    
//Fixture
class UpdateSubscriptionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize required variables
        std::string eoPath = "/test/path";
        bool authEnabled = false;

        // Initialize the gRPC server
        grpc::ServerBuilder builder;
        cq = builder.AddCompletionQueue(); // Add a completion queue

        // Create the service instance
        service = new CatenaServiceImpl(cq.get(), mockDevice, eoPath, authEnabled);

        // Register the service with the server
        builder.RegisterService(service);

        // Build and start the server
        server = builder.BuildAndStart();
    }

    void TearDown() override {
        // Shutdown the server first to stop accepting new requests
        server->Shutdown();
    
        // Drain the completion queue with a timeout
        void* tag;
        bool ok;
        int max_iterations = 100; // Limit the number of iterations
        for (int i = 0; i < max_iterations; ++i) {
            auto status = cq->AsyncNext(&tag, &ok, gpr_time_from_seconds(1, GPR_TIMESPAN));
            if (ok == false || status == grpc::CompletionQueue::SHUTDOWN) {
                break;
            } 
        }
    
        // Shutdown the completion queue
        cq->Shutdown();
    
        // Clean up resources
        delete service;
        cq.reset();
        server.reset();
    }

    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    std::unique_ptr<grpc::Server> server;
    MockDevice mockDevice;
    MockSubscriptionManager mockSubscriptionManager;
    CatenaServiceImpl* service;
    grpc::Service mockService; // Optional: Mock service for testing
    
};

TEST_F(UpdateSubscriptionsTest, HandlesSubscriptionAdditions) {
    // Replace the real service with the mock
    MockCatenaServiceImpl mockService;

    // Mock the writer
    MockServerAsyncWriter mockWriter;

    // Set expectations for the mock writer
    EXPECT_CALL(mockWriter, Write(_, _))
        .Times(1)
        .WillOnce(Invoke([](const catena::DeviceComponent_ComponentParam& msg, void* tag) {
            std::cerr << "[          ] mockWriter " << std::endl;
        }));

    // Set expectations for the mock method
    EXPECT_CALL(mockService, RequestUpdateSubscriptions(_, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([&mockWriter](grpc::ServerContext* context,
                                       catena::UpdateSubscriptionsPayload* request,
                                       grpc::ServerAsyncWriter<catena::DeviceComponent_ComponentParam>* writer,
                                       grpc::ServerCompletionQueue* cq,
                                       grpc::ServerCompletionQueue* notification_cq,
                                       void* tag) {
            std::cerr << "[          ] mockService " << std::endl;
            // Simulate a response being written
            catena::DeviceComponent_ComponentParam response;
            mockWriter.Write(response, tag);
        }));

    // Simulate a gRPC call
    grpc::ServerContext context;
    catena::UpdateSubscriptionsPayload request;

    // Call the mocked method
    mockService.RequestUpdateSubscriptions(&context, &request, nullptr, nullptr, nullptr, nullptr);

    // Verify that the test completes without hanging
    SUCCEED();
}
