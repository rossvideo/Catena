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
 * @brief This file is for testing the Subscriptions.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/13
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "RESTTest.h"
#include "MockSocketReader.h"
#include "MockParam.h"
#include "MockDevice.h"
#include "MockSubscriptionManager.h"

// REST
#include "controllers/Subscriptions.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTSubscriptionsTests : public ::testing::Test, public RESTTest {
  protected:
    RESTSubscriptionsTests() : RESTTest(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        EXPECT_CALL(context, slot()).WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(context, method()).WillRepeatedly(::testing::ReturnRef(testMethod));
        EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(testToken));
        EXPECT_CALL(context, jsonBody()).WillRepeatedly(::testing::ReturnRef(testJsonBody));
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, getSubscriptionManager()).WillRepeatedly(::testing::ReturnRef(subManager));

        EXPECT_CALL(subManager, getAllSubscribedOids(::testing::_))
            .WillRepeatedly(::testing::Invoke([this](const catena::common::IDevice &dm){
                // Make sure device is passed in.
                EXPECT_EQ(&dm, &this->dm);
                return std::set<std::string>(oids.begin(), oids.end());
            }));

        EXPECT_CALL(dm, subscriptions()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMutex));

        // Creating subscriptions object.
        endpoint = Subscriptions::makeOne(serverSocket, context, dm);

        // Initializing test values.
        for (size_t i = 0; i < oids.size(); ++i) {
            // Adding variables to the back of the vectors.
            params.emplace_back();
            params.back() = std::make_unique<MockParam>();
            responses.emplace_back();
            responses.back().set_oid(oids[i]);
            responses.back().mutable_param()->mutable_value()->set_string_value("value" + std::to_string(i + 1));
            responsesJson.emplace_back();
            auto status = google::protobuf::util::MessageToJsonString(responses.back(), &responsesJson.back());
            EXPECT_TRUE(status.ok());
            // Setting default behaviours.
            EXPECT_CALL(dm, getParam(oids[i], ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this, i](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
                    // Make sure authz is correctly passed in.
                    if (authzEnabled) {
                        EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
                    } else {
                        EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
                    }
                    return std::move(params[i]);
                }));
            EXPECT_CALL(*params.back(), getOid()).WillRepeatedly(::testing::ReturnRefOfCopy(oids[i]));
            EXPECT_CALL(*params.back(), toProto(::testing::An<catena::Param&>(), ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this, i](catena::Param &param, catena::common::Authorizer &authz) {
                    // Make sure authz is correctly passed in.
                    if (authzEnabled) {
                        EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
                    } else {
                        EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
                    }
                    param.CopyFrom(responses[i].param());
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
        }
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (endpoint) {
            delete endpoint;
        }
    }
    
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::string testMethod = "GET";
    std::string testLanguage = "en";
    std::string testToken = "";
    std::string testJsonBody;
    bool authzEnabled = false;
    
    MockSocketReader context;
    std::mutex mockMutex;
    MockDevice dm;
    MockSubscriptionManager subManager;
    catena::REST::ICallData* endpoint = nullptr;

    // Test values.
    std::vector<std::string> oids{"param1", "param2"};
    std::vector<std::unique_ptr<MockParam>> params;
    std::vector<catena::DeviceComponent_ComponentParam> responses;
    std::vector<std::string> responsesJson;
};

/* 
 * TEST 0.1 - Creating a Subscriptions object with makeOne.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_create) {
    // Making sure subscriptions is created from the SetUp step.
    ASSERT_TRUE(endpoint);
}

/* 
 * TEST 0.2 - Writing to console with Subscriptions finish().
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_Finish) {
    // Calling finish and expecting the console output.
    endpoint->finish();
    ASSERT_TRUE(MockConsole.str().find("Subscriptions[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - Subscriptions with a device which does not support them.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_NotSupported) {
    catena::exception_with_status rc("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);

    // Setting expectations.
    EXPECT_CALL(dm, subscriptions()).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, getSubscriptionManager()).Times(0);

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 0.4 - Subscriptions with an invalid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_AuthzInalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testToken = "Invalid token";

    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    // Should not make it past authz.
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(context, getSubscriptionManager()).Times(0);

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 0.5 - Subscriptions with an invalid method.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_BadMethod) {
    catena::exception_with_status rc("Bad method", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "BAD_METHOD";

    // Setting expectations. Should not call.
    EXPECT_CALL(context, getSubscriptionManager()).Times(0);

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}


/*
 * ============================================================================
 *                              GET Subscriptions tests
 * ============================================================================
 * 
 * TEST 1.1 - GET Subscriptions normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GetNormal) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}

/* 
 * TEST 1.2 - GET Subscriptions with a valid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GetAuthzValid) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    authzEnabled = true;
    testToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";

    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}

/* 
 * TEST 1.3 - GET Subscriptions fail to retrieve a param.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GetGetParamReturnErr) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    oids.insert(oids.begin(), "errParam");

    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            status = catena::exception_with_status("Param not found", catena::StatusCode::NOT_FOUND);
            return nullptr; // Simulating an error.
        }));
    
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}

/* 
 * TEST 1.4 - GET Subscriptions fails to convert param to proto.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GetToProtoReturnErr) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    oids.insert(oids.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();

    EXPECT_CALL(subManager, getAllSubscribedOids(::testing::_))
        .WillOnce(::testing::Return(std::set<std::string>(oids.begin(), oids.end())));
    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(::testing::ReturnRef(oids[0]));
    EXPECT_CALL(*errParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).WillRepeatedly(::testing::Invoke(
        [](catena::Param &param, catena::common::Authorizer &authz) {
            // Simulating an error in conversion.
            return catena::exception_with_status("Failed to convert to proto", catena::StatusCode::UNKNOWN);
        }));
    
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}


