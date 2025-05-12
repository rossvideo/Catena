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
 * @brief This file is for testing the GetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/09
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

#include "SocketHelper.h"

#include <IDevice.h>
#include "controllers/GetValue.h"
#include "interface/ISocketReader.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Mocking the SocketReader interface
class MockSocketReader : public ISocketReader {
  public:
    MOCK_METHOD(void, read, (tcp::socket& socket, bool authz), (override));
    MOCK_METHOD(const std::string&, method, (), (const, override));
    MOCK_METHOD(const std::string&, service, (), (const, override));
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

// Fixture
class RESTGetValueTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTGetValueTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Setting expectations of 1 call to the constructor and one to origin().
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        getValue = GetValue::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (getValue) {
            delete getValue;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* getValue = nullptr;
};

/*
 * ============================================================================
 *                               GetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetValue object with makeOne.
 */
TEST_F(RESTGetValueTests, GetValue_create) {
    // Making sure getValue is created from the SetUp step.
    ASSERT_TRUE(getValue);
}

/* 
 * TEST 2 - Normal case for GetValue proceed().
 */
TEST_F(RESTGetValueTests, GetValue_proceedNormal) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, fields("oid")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // dm.getValue()
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
            value.CopyFrom(returnVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    getValue->proceed();

    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 3 - dm.getValue() returns an catena::exception_with_status.
 */
TEST_F(RESTGetValueTests, GetValue_proceedErrReturnCatena) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/invalid_oid";

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, fields("oid")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // dm.getValue()
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    getValue->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - GetValue with authz on and valid token.
 */
TEST_F(RESTGetValueTests, GetValue_proceedAuthzValid) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::string mockOid = "/test_oid";
    /* Authz just tests for a properly encrypted token, proxy handles authz.
     * This is a random RSA token I made jwt.io it is not a security risk I
     * swear. */
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

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    EXPECT_CALL(context, fields("oid")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // dm.getValue()
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
            value.CopyFrom(returnVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    getValue->proceed();

    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 5 - GetValue with authz on and an invalid token.
 */
TEST_F(RESTGetValueTests, GetValue_proceedAuthzInvalid) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    std::string mockToken = "THIS SHOULD NOT PARSE";

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    // Should NOT make it this far.
    EXPECT_CALL(context, fields("oid")).Times(0);
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    getValue->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 6 - dm.getValue() throws a catena::exception_with_status.
 */
TEST_F(RESTGetValueTests, GetValue_proceedErrThrowCatena) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/invalid_oid";

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, fields("oid")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // dm.getValue()
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Calling proceed() and checking written response.
    getValue->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 7 - dm.getValue() throws an std::runtime_error.
 */
TEST_F(RESTGetValueTests, GetValue_proceedErrThrowUnknown) {
    // Setting up the returnVal to test with.
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::UNKNOWN);
    std::string mockOid = "/invalid_oid";

    // Defining mock functions
    // authz = false, fields("oid") = "/text_oid"
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, fields("oid")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // dm.getValue()
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
            throw std::runtime_error("Unknown error");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Calling proceed() and checking written response.
    getValue->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 8 - Writing to console with GetValue finish().
 */
TEST_F(RESTGetValueTests, GetValue_finish) {
    // Calling finish and expecting the console output.
    getValue->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("GetValue[7] finished\n") != std::string::npos);
}
