#include <gtest/gtest.h>
#include "controllers/Connect.h"
#include "RESTMockClasses.h"
#include "../../common/tests/CommonMockClasses.h"
#include "SocketHelper.h"
#include <boost/asio.hpp>

using namespace catena::REST;
using namespace catena::common;
using namespace testing;

class ConnectTest : public ::testing::Test, public SocketHelper {
protected:
    ConnectTest() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Create mock objects
        socket_reader_ = std::make_unique<MockSocketReader>();
        device_ = std::make_unique<MockDevice>();
        subscription_manager_ = std::make_unique<MockSubscriptionManager>();
        
        // Set up common expectations
        setupCommonExpectations();
        
        // Create Connect instance
        connect_ = std::make_unique<catena::REST::Connect>(serverSocket, *socket_reader_, *device_);
    }

    void TearDown() override {
        connect_.reset();
        socket_reader_.reset();
        device_.reset();
    }

    // Helper function to set up common mock expectations
    void setupCommonExpectations() {
        EXPECT_CALL(*socket_reader_, detailLevel())
            .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));
        EXPECT_CALL(*device_, slot())
            .WillRepeatedly(Return(1));
        EXPECT_CALL(*socket_reader_, getSubscriptionManager())
            .WillRepeatedly(ReturnRef(*subscription_manager_));
        EXPECT_CALL(*socket_reader_, origin())
            .WillRepeatedly(ReturnRef(origin));
        EXPECT_CALL(*socket_reader_, fields("user_agent"))
            .WillRepeatedly(ReturnRef(user_agent_));
        EXPECT_CALL(*socket_reader_, hasField("force_connection"))
            .WillRepeatedly(Return(false));
    }

    // Helper function to build slot response
    std::string buildSlotResponse(uint32_t slot) {
        catena::PushUpdates populatedSlots;
        populatedSlots.set_slot(slot);
        std::string slotJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(populatedSlots, &slotJson, options);
        return slotJson;
    }

    // Helper function to build parameter update response
    std::string buildParamUpdateResponse(uint32_t slot, const std::string& oid, const std::string& value) {
        catena::PushUpdates updateResponse;
        updateResponse.set_slot(slot);
        auto* paramValue = updateResponse.mutable_value();
        paramValue->set_oid(oid);
        auto* valueObj = paramValue->mutable_value();
        valueObj->set_string_value(value);
        std::string updateJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(updateResponse, &updateJson, options);
        return updateJson;
    }

    // Helper function to build language pack update response
    std::string buildLanguagePackUpdateResponse(uint32_t slot, const std::string& name, const std::map<std::string, std::string>& words) {
        catena::PushUpdates updateResponse;
        updateResponse.set_slot(slot);
        auto* deviceComponent = updateResponse.mutable_device_component();
        auto* languagePackComponent = deviceComponent->mutable_language_pack();
        auto* pack = languagePackComponent->mutable_language_pack();
        pack->set_name(name);
        for (const auto& [key, value] : words) {
            (*pack->mutable_words())[key] = value;
        }
        std::string updateJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(updateResponse, &updateJson, options);
        return updateJson;
    }
    std::unique_ptr<MockSocketReader> socket_reader_;
    std::unique_ptr<MockDevice> device_;
    std::unique_ptr<catena::REST::Connect> connect_;
    std::unique_ptr<MockSubscriptionManager> subscription_manager_;
    std::string user_agent_ = "test_agent";
    std::string paramOid_ = "test_param";
    std::string paramScope_ = "test_scope";
};

// --- 0. INITIAL TESTS ---

// Test 0.1: Test constructor initialization
TEST_F(ConnectTest, ConstructorInitialization) {
    // Verify that the Connect object was created successfully
    ASSERT_NE(connect_, nullptr);
}

// Test 0.2: Test unauthorized connection
TEST_F(ConnectTest, ProceedHandlesAuthzError) {
    std::string mockToken = "invalid_token";
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);

    // Set up expectations for authorization error
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    
    // Call proceed and expect it to handle the error
    connect_->proceed();

    // Verify the error response
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc));
}

// Test 0.3: Test authorized connection
TEST_F(ConnectTest, ProceedHandlesValidAuthz) {
    // Use a valid JWT token that was borrowed from GetValue_test.cpp
    std::string mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
                            "OiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2Nvc"
                            "GUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4Om"
                            "NmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMiw"
                            "ibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dTo"
                            "krEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko965"
                            "3v0khyUT4UKeOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKH"
                            "kWi4P3-CYWrwe-g6b4-a33Q0k6tSGI1hGf2bA9cRYr-VyQ_T3"
                            "RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEmuIwNOVM3EcVEaL"
                            "yISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg_w"
                            "bOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9M"
                            "dvJH-cx1s146M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    // Set up the expected response
    std::string slotJson = buildSlotResponse(1);

    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Give proceed() time to start and send its response
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Verify the initial response
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson}));

    // Emit shutdown signal to break out of the wait loop
    catena::REST::Connect::shutdownSignal_.emit();

    // Clean up
    serverSocket.close();
    proceed_thread.join();
}

// I am not testing exception handling as it is improved in a seperate PR!

// --- 1. SIGNAL TESTS ---

// Test 1.1: Test value set by server signal
TEST_F(ConnectTest, HandlesValueSetByServer) {
    // Use the monitor scope token from Authorization_test.cpp
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    // Create a mock parameter with monitor scope
    auto param = std::make_unique<MockParam>();
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(ReturnRef(paramOid_));
    EXPECT_CALL(*param, getScope())
        .WillRepeatedly(ReturnRef(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(*param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::Value& value, catena::common::Authorizer&) {
            value.set_string_value("test_value");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Set up expectation for updateResponse_ call
    std::set<std::string> subscribed_oids{paramOid_};
    EXPECT_CALL(*subscription_manager_, getAllSubscribedOids(::testing::Ref(*device_)))
        .WillOnce(ReturnRef(subscribed_oids));

    // Set up the expected responses
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildParamUpdateResponse(1, paramOid_, "test_value");

    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Emit signal and wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    device_->valueSetByServer.emit(paramOid_, param.get(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Read the combined response (both slot and update)
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    // Emit shutdown signal to break out of the wait loop
    catena::REST::Connect::shutdownSignal_.emit();

    // Clean up
    serverSocket.close();
    proceed_thread.join();
}

// Test 1.2: Test value set by client signal
TEST_F(ConnectTest, HandlesValueSetByClient) {
    // Use the monitor scope token from Authorization_test.cpp
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    // Create a mock parameter with monitor scope
    auto param = std::make_unique<MockParam>();
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(ReturnRef(paramOid_));
    EXPECT_CALL(*param, getScope())
        .WillRepeatedly(ReturnRef(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(*param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::Value& value, catena::common::Authorizer&) {
            value.set_string_value("test_value");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Set up expectation for updateResponse_ call
    std::set<std::string> subscribed_oids{paramOid_};
    EXPECT_CALL(*subscription_manager_, getAllSubscribedOids(::testing::Ref(*device_)))
        .WillOnce(ReturnRef(subscribed_oids));

    // Set up the expected responses
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildParamUpdateResponse(1, paramOid_, "test_value");

    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Emit signal and wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    device_->valueSetByServer.emit(paramOid_, param.get(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Read the combined response (both slot and update)
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    // Emit shutdown signal to break out of the wait loop
    catena::REST::Connect::shutdownSignal_.emit();

    // Clean up
    serverSocket.close();
    proceed_thread.join();
}

// Test 1.3: Test language signal
TEST_F(ConnectTest, HandlesLanguage) {
    // Use the monitor scope token from Authorization_test.cpp
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    // Create a mock language pack
    auto languagePack = std::make_unique<MockLanguagePack>();
    EXPECT_CALL(*languagePack, toProto(::testing::_))
        .WillOnce(::testing::Invoke([](catena::LanguagePack& pack) {
            pack.set_name("English");
            (*pack.mutable_words())["greeting"] = "Hello";
        }));

    // Set up the expected responses
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildLanguagePackUpdateResponse(1, "English", {{"greeting", "Hello"}});

    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Emit signal and wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    device_->languageAddedPushUpdate.emit(languagePack.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Read the combined response (both slot and update)
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    // Emit shutdown signal to break out of the wait loop
    catena::REST::Connect::shutdownSignal_.emit();

    // Clean up
    serverSocket.close();
    proceed_thread.join();
}

// --- 2. FINISH TESTS ---

// Test 2.1: Test normal finish behavior
TEST_F(ConnectTest, FinishClosesConnection) {
    static const std::string empty_token;
    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(false));  // No auth needed for this test
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(empty_token));  // Empty token since auth is disabled
    
    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Give proceed() time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Call finish
    connect_->finish();

    // Verify socket is closed
    EXPECT_FALSE(serverSocket.is_open());

    // Clean up
    proceed_thread.join();
}

// Test 2.2: Test finish with active signal handlers
TEST_F(ConnectTest, FinishDisconnectsSignalHandlers) {
    static const std::string empty_token;
    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(false));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(empty_token));  // Empty token since auth is disabled
    
    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Give proceed() time to start and set up signal handlers
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Call finish
    connect_->finish();

    // Verify socket is closed
    EXPECT_FALSE(serverSocket.is_open());

    // Verify signal handlers are disconnected by attempting to emit signals
    // These should not cause any updates since handlers are disconnected
    auto param = std::make_unique<MockParam>();
    device_->valueSetByServer.emit("test_oid", param.get(), 0);
    device_->valueSetByClient.emit("test_oid", param.get(), 0);
    
    // No response should be received since handlers are disconnected
    EXPECT_EQ(readResponse(), "");

    // Clean up
    proceed_thread.join();
}

// Test 2.3: Test finish sends final OK response
TEST_F(ConnectTest, FinishSendsFinalResponse) {
    static const std::string empty_token;
    // Set up expectations for valid authorization
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(false));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(empty_token));  // Empty token since auth is disabled
    
    // Run proceed() in a separate thread
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });

    // Give proceed() time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Call finish
    connect_->finish();

    // Verify final OK response is sent
    catena::exception_with_status rc("", catena::StatusCode::OK);
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc));

    // Clean up
    proceed_thread.join();
}
