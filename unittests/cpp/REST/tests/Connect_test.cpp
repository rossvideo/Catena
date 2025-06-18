#include <gtest/gtest.h>
#include "controllers/Connect.h"
#include "MockSocketReader.h"
#include "MockDevice.h"
#include "RESTTest.h"
#include "CommonTestHelpers.h"
#include "MockSubscriptionManager.h"
#include "MockLanguagePack.h"
#include <boost/asio.hpp>

using namespace catena::REST;
using namespace catena::common;
using namespace testing;

class ConnectTest : public ::testing::Test, public RESTTest {
protected:
    ConnectTest() : RESTTest(&serverSocket, &clientSocket) {}

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
    ASSERT_NE(connect_, nullptr);
}

// Test 0.2: Test unauthorized connection
TEST_F(ConnectTest, ProceedHandlesAuthzError) {
    std::string mockToken = "invalid_token";
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    
    connect_->proceed();
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc));
}

// Test 0.3: Test authorized connection
TEST_F(ConnectTest, ProceedHandlesValidAuthz) {
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

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    std::string slotJson = buildSlotResponse(1);

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson}));

    catena::REST::Connect::shutdownSignal_.emit();
    serverSocket.close();
    proceed_thread.join();
}

// --- 1. SIGNAL TESTS ---

// Test 1.1: Test value set by server signal
TEST_F(ConnectTest, HandlesValueSetByServer) {
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

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

    std::set<std::string> subscribed_oids{paramOid_};
    EXPECT_CALL(*subscription_manager_, getAllSubscribedOids(::testing::Ref(*device_)))
        .WillRepeatedly(Return(subscribed_oids));

    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildParamUpdateResponse(1, paramOid_, "test_value");

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    device_->valueSetByServer.emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    catena::REST::Connect::shutdownSignal_.emit();
    serverSocket.close();
    proceed_thread.join();
}

// Test 1.2: Test value set by client signal
TEST_F(ConnectTest, HandlesValueSetByClient) {
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

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

    std::set<std::string> subscribed_oids{paramOid_};
    EXPECT_CALL(*subscription_manager_, getAllSubscribedOids(::testing::Ref(*device_)))
        .WillRepeatedly(Return(subscribed_oids));

    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildParamUpdateResponse(1, paramOid_, "test_value");

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    device_->valueSetByClient.emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    catena::REST::Connect::shutdownSignal_.emit();
    serverSocket.close();
    proceed_thread.join();
}

// Test 1.3: Test language signal
TEST_F(ConnectTest, HandlesLanguage) {
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

    auto languagePack = std::make_unique<MockLanguagePack>();
    EXPECT_CALL(*languagePack, toProto(::testing::_))
        .WillOnce(::testing::Invoke([](catena::LanguagePack& pack) {
            pack.set_name("English");
            (*pack.mutable_words())["greeting"] = "Hello";
        }));

    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string slotJson = buildSlotResponse(1);
    std::string updateJson = buildLanguagePackUpdateResponse(1, "English", {{"greeting", "Hello"}});

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    device_->languageAddedPushUpdate.emit(languagePack.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {slotJson, updateJson}));

    catena::REST::Connect::shutdownSignal_.emit();
    serverSocket.close();
    proceed_thread.join();
}

// --- 2. FINISH TESTS ---

// Test 2.1: Test normal finish behavior
TEST_F(ConnectTest, FinishClosesConnection) {
    static const std::string empty_token;
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(false));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(empty_token));
    
    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    connect_->finish();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_FALSE(serverSocket.is_open());

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();
}

// Test 2.2: Test finish with active signal handlers
TEST_F(ConnectTest, FinishDisconnectsSignalHandlers) {
    static const std::string empty_token;
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(false));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(empty_token));
    
    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::string slotJson = buildSlotResponse(1);
    connect_->finish();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_FALSE(serverSocket.is_open());

    // Verify signal handlers are disconnected by attempting to emit signals
    auto param = std::make_unique<MockParam>();
    device_->valueSetByServer.emit("test_oid", param.get());
    device_->valueSetByClient.emit("test_oid", param.get());
    
    EXPECT_EQ(readResponse(), expectedSSEResponse(catena::exception_with_status("", catena::StatusCode::OK), {slotJson}));

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();
}

// --- 3. EXCEPTION TESTS ---

// Test 3.1: Test catena::exception_with_status handling
TEST_F(ConnectTest, ProceedHandlesCatenaException) {
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(Invoke([]() -> const std::string& {
            static const std::string empty;
            throw catena::exception_with_status("Auth error", catena::StatusCode::UNAUTHENTICATED);
            return empty;
        }));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(catena::exception_with_status("Auth error", catena::StatusCode::UNAUTHENTICATED)));

    serverSocket.close();
    proceed_thread.join();
}

// Test 3.2: Test std::exception handling
TEST_F(ConnectTest, ProceedHandlesStdException) {
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(Invoke([]() -> const std::string& {
            static const std::string empty;
            throw std::runtime_error("Runtime error");
            return empty;
        }));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(
        catena::exception_with_status("Connection setup failed: Runtime error", catena::StatusCode::INTERNAL)));

    serverSocket.close();
    proceed_thread.join();
}

// Test 3.3: Test unknown exception handling
TEST_F(ConnectTest, ProceedHandlesUnknownException) {
    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(Invoke([]() -> const std::string& {
            static const std::string empty;
            throw 42; // Throw an int to trigger catch(...)
            return empty;
        }));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    EXPECT_EQ(readResponse(), expectedSSEResponse(
        catena::exception_with_status("Unknown error during connection setup", catena::StatusCode::UNKNOWN)));

    serverSocket.close();
    proceed_thread.join();
}

// Test 3.4: Test socket close during response sending with writer failure
TEST_F(ConnectTest, ProceedHandlesWriterFailure) {
    std::string mockToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxM"
                            "jM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJ"
                            "zdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxst"
                            "pXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";

    EXPECT_CALL(*socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(*socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    EXPECT_CALL(*device_, slot())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*socket_reader_, detailLevel())
        .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));

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

    std::set<std::string> subscribed_oids{paramOid_};
    EXPECT_CALL(*subscription_manager_, getAllSubscribedOids(::testing::Ref(*device_)))
        .WillRepeatedly(Return(subscribed_oids));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        connect_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    clientSocket.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    device_->valueSetByServer.emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_FALSE(serverSocket.is_open());

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();
}
