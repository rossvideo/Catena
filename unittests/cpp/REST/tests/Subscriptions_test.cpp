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
 * @date 25/06/16
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

    /*
     * Redirects cout and sets default expectations for each method.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Default expectations for the context.
        EXPECT_CALL(context, method()).WillRepeatedly(::testing::ReturnRef(testMethod));
        EXPECT_CALL(context, slot()).WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(testToken));
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, jsonBody()).WillRepeatedly(::testing::ReturnRef(testJsonBody));
        EXPECT_CALL(context, getSubscriptionManager()).WillRepeatedly(::testing::ReturnRef(subManager));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Invoke([this](){ return authzEnabled; }));
        EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(false));

        // Default expectations for subscription manger.
        EXPECT_CALL(subManager, getAllSubscribedOids(::testing::_))
            .WillRepeatedly(::testing::Invoke([this](const catena::common::IDevice &dm){
                // Make sure device is passed in.
                EXPECT_EQ(&dm, &this->dm);
                return std::set<std::string>(oids.begin(), oids.end());
            }));

        // default expectations for the device model.
        EXPECT_CALL(dm, subscriptions()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMutex));

        // Default expectations for each test param.
        for (size_t i = 0; i < oids.size(); ++i) {
            // Initializing test parameters and their expected responses.
            params.emplace_back();
            params.back() = std::make_unique<MockParam>();
            responses.emplace_back();
            responses.back().set_oid(oids[i]);
            responses.back().mutable_param()->mutable_value()->set_string_value("value" + std::to_string(i + 1));
            responsesJson.emplace_back();
            auto status = google::protobuf::util::MessageToJsonString(responses.back(), &responsesJson.back());
            EXPECT_TRUE(status.ok());

            // Default expectations for test params for GET calls.
            EXPECT_CALL(dm, getParam(oids[i], ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this, i](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
                    // Make sure authz is correctly passed in.
                    EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    return std::move(params[i]);
                }));
            EXPECT_CALL(*params.back(), getOid()).WillRepeatedly(::testing::ReturnRefOfCopy(oids[i]));
            EXPECT_CALL(*params.back(), toProto(::testing::An<catena::Param&>(), ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this, i](catena::Param &param, catena::common::Authorizer &authz) {
                    // Make sure authz is correctly passed in.
                    EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    param.CopyFrom(responses[i].param());
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));

            // Default expectations for test params for PUT calls.
            EXPECT_CALL(subManager, removeSubscription(oids[i], ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
                    // Make sure device is correctly passed in.
                    EXPECT_EQ(&dm, &this->dm);
                    this->removed_oids++;
                    return true;
                }));
            EXPECT_CALL(subManager, addSubscription(oids[i], ::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
                [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
                    // Make sure authz and device are correctly passed in.
                    EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    EXPECT_EQ(&dm, &this->dm);
                    this->added_oids++;
                    return true;
                }));
        }
    }

    /*
     * Helper to initialize a jsonBody via UpdateSubscriptionsPayload.
     */
    void initPayload(const std::vector<std::string> &addOids, const std::vector<std::string> &remOids) {
        catena::UpdateSubscriptionsPayload payload;
        // Adding addOids.
        for (const auto& oid : addOids) { payload.add_added_oids(oid); }
        // Adding remOids.
        for (const auto& oid : remOids) { payload.add_removed_oids(oid); }
        // Converting to JSON body.
        auto status = google::protobuf::util::MessageToJsonString(payload, &testJsonBody);
        EXPECT_TRUE(status.ok());
    }

    /*
     * Restores cout and deletes endpoint.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (endpoint) {
            delete endpoint;
        }
    }
    
    // Cout variables.
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    // Context variables.
    std::string testMethod = "GET";
    std::string testToken = "";
    std::string testJsonBody;
    bool authzEnabled = false;
    // Mock objects and endpoint.
    MockSocketReader context;
    std::mutex mockMutex;
    MockDevice dm;
    MockSubscriptionManager subManager;
    catena::REST::ICallData* endpoint = nullptr;
    // Test params.
    std::vector<std::string> oids{"param1", "param2"};
    std::vector<std::unique_ptr<MockParam>> params;
    std::vector<catena::DeviceComponent_ComponentParam> responses;
    std::vector<std::string> responsesJson;
    // Trackers for calls to add/remove subscriptions.
    uint32_t added_oids = 0;
    uint32_t removed_oids = 0;
};

/* 
 * TEST 0.1 - Creating a Subscriptions object with makeOne.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_create) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    ASSERT_TRUE(endpoint);
}

/* 
 * TEST 0.2 - Writing to console with Subscriptions finish().
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_Finish) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    endpoint->finish();
    ASSERT_TRUE(MockConsole.str().find("Subscriptions[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - Subscriptions with a device which does not support them.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_NotSupported) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);
    // Setting expectations.
    EXPECT_CALL(dm, subscriptions()).WillOnce(::testing::Return(false));
    EXPECT_CALL(context, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 0.4 - Subscriptions with an invalid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_AuthzInalid) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testToken = "Invalid token";
    authzEnabled = true;
    // Setting expectations.
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(context, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 0.5 - Subscriptions with an invalid method.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_BadMethod) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc("Bad method", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "BAD_METHOD";
    // Setting expectations.
    EXPECT_CALL(context, getSubscriptionManager()).Times(0); // Should not call.
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
TEST_F(RESTSubscriptionsTests, Subscriptions_GETNormal) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}

/* 
 * TEST 1.2 - GET Stream normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETStream) {
    // Enabling stream.
    EXPECT_CALL(context, stream()).WillOnce(::testing::Return(true));
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, responsesJson));
}

/* 
 * TEST 1.3 - GET Subscriptions with a valid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETAuthzValid) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
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
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, responsesJson));
}

/* 
 * TEST 1.4 - GET Subscriptions fail to retrieve a param.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamReturnErr) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    oids.insert(oids.begin(), "errParam");
    // Setting expectations.
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
 * TEST 1.5 - GET Subscriptions call to GetParam throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamThrowCatena) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Param not found", catena::StatusCode::NOT_FOUND};
    oids.insert(oids.begin(), "errParam");
    // Setting expectations.
    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [&rc](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr; // Simulating an error.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.6 - GET Subscriptions call to GetParam throws an std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamThrowUnknown) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Unknown error", catena::StatusCode::UNKNOWN};
    oids.insert(oids.begin(), "errParam");
    // Setting expectations.
    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            throw std::runtime_error("Unknown error occurred");
            return nullptr; // Simulating an error.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.7 - GET Subscriptions fails to convert param to proto.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoReturnErr) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    oids.insert(oids.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();
    // Setting expectations.
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

/* 
 * TEST 1.8 - GET Subscriptions call to toProto throws an catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoThrowCatena) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Param not found", catena::StatusCode::NOT_FOUND};
    oids.insert(oids.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();

    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(::testing::ReturnRef(oids[0]));
    EXPECT_CALL(*errParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).WillRepeatedly(::testing::Invoke(
        [&rc](catena::Param &param, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.9 - GET Subscriptions call to toProto throws an std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoThrowUnknown) {
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Unknown error", catena::StatusCode::UNKNOWN};
    oids.insert(oids.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();
    // Setting expectations.
    EXPECT_CALL(dm, getParam("errParam", ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(::testing::ReturnRef(oids[0]));
    EXPECT_CALL(*errParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).WillRepeatedly(::testing::Invoke(
        [](catena::Param &param, catena::common::Authorizer &authz) {
            // Simulating an error in conversion.
            throw std::runtime_error("Unknown error");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              PUT Subscriptions tests
 * ============================================================================
 * 
 * TEST 2.1 - PUT Subscriptions normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTNormal) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    initPayload({"param1", "param2"}, {"param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 2); // Both oids should be added.
    EXPECT_EQ(removed_oids, 2); // Both oids should be removed.
}

/* 
 * TEST 2.2 - PUT Subscriptions with a valid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAuthzValid) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    initPayload({"param1", "param2"}, {"param1", "param2"});
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
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 2); // Both oids should be added.
    EXPECT_EQ(removed_oids, 2); // Both oids should be removed.
}

/* 
 * TEST 2.3 - PUT Subscriptions normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTFailParse) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc("Failed to parse JSON Body", catena::StatusCode::INVALID_ARGUMENT);
    testJsonBody = "Not a JSON string";
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 0); // No oids should be added.
    EXPECT_EQ(removed_oids, 0); // No oids should be removed.
}

/* 
 * TEST 2.4 - PUT Subscriptions add and remove return an error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTReturnErr) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    initPayload({"errParam", "param1", "param2"}, {"errParam", "param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    EXPECT_CALL(subManager, removeSubscription("errParam", ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            rc = catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    EXPECT_CALL(subManager, addSubscription("errParam", ::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
            // Simulating an error in adding subscription.
            rc = catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 2); // All oids but errParam should be added.
    EXPECT_EQ(removed_oids, 2); // All oids but errParam should be removed.
}

/* 
 * TEST 2.5 - PUT Subscriptions remove throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTRemThrowCatena) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT};
    initPayload({}, {"errParam", "param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    EXPECT_CALL(subManager, removeSubscription("errParam", ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [testRc = &rc](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            throw catena::exception_with_status(testRc->what(), testRc->status);
            return false;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 0); // No oids should be added.
    EXPECT_EQ(removed_oids, 0); // No oids should be removed.
}

/* 
 * TEST 2.6 - PUT Subscriptions remove throws a std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTRemThrowUnknown) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Unknown error", catena::StatusCode::UNKNOWN};
    initPayload({}, {"errParam", "param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    EXPECT_CALL(subManager, removeSubscription("errParam", ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [testRc = &rc](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            throw std::runtime_error(testRc->what());
            return false;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 0); // No oids should be added.
    EXPECT_EQ(removed_oids, 0); // No oids should be removed.
}

/* 
 * TEST 2.7 - PUT Subscriptions add throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAddThrowCatena) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT};
    initPayload({"errParam", "param1", "param2"}, {});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    EXPECT_CALL(subManager, addSubscription("errParam", ::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [testRc = &rc](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
            // Simulating an error in adding subscription.
            throw catena::exception_with_status(testRc->what(), testRc->status);
            return false;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 0); // No oids should be added.
    EXPECT_EQ(removed_oids, 0); // No oids should be removed.
}

/* 
 * TEST 2.8 - PUT Subscriptions add throws a std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAddThrowUnknown) {
    testMethod = "PUT";
    endpoint = Subscriptions::makeOne(serverSocket, context, dm);
    catena::exception_with_status rc{"Unknown error", catena::StatusCode::UNKNOWN};
    initPayload({"errParam", "param1", "param2"}, {});
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz false
    EXPECT_CALL(subManager, addSubscription("errParam", ::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [testRc = &rc](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
            // Simulating an error in adding subscription.
            throw std::runtime_error(testRc->what());
            return false;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
    EXPECT_EQ(added_oids, 0); // No oids should be added.
    EXPECT_EQ(removed_oids, 0); // No oids should be removed.
}
