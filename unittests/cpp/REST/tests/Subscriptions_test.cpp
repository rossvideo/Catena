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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

// Test helpers
#include "RESTTest.h"
#include "MockParam.h"
#include "MockSubscriptionManager.h"

// REST
#include "controllers/Subscriptions.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTSubscriptionsTests : public RESTEndpointTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTSubscriptionsTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    /*
     * Sets default expectations for the Subscriptions tests.
     */
    RESTSubscriptionsTests() : RESTEndpointTest() {
        // Default expectations for context_.
        EXPECT_CALL(context_, getSubscriptionManager()).WillRepeatedly(testing::ReturnRef(subManager_));
        // Default expectations for device model.
        EXPECT_CALL(dm0_, subscriptions()).WillRepeatedly(testing::Return(true));
        // Default expectations for device model 1 (do not call).
        EXPECT_CALL(dm1_, subscriptions()).Times(0);
        EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
        // Default expectations for sub manager.
        EXPECT_CALL(subManager_, getAllSubscribedOids(testing::_))
            .WillRepeatedly(testing::Invoke([this](const catena::common::IDevice &dm){
                // Make sure device is passed in.
                EXPECT_EQ(&dm, &dm0_);
                return std::set<std::string>(oids_.begin(), oids_.end());
            }));
        // Default expectations for each test param.
        for (size_t i = 0; i < oids_.size(); ++i) {
            // Initializing test parameters and their expected responses_.
            params_.emplace_back();
            params_.back() = std::make_unique<MockParam>();
            responses_.emplace_back();
            responses_.back().set_oid(oids_[i]);
            responses_.back().mutable_param()->mutable_value()->set_string_value("value" + std::to_string(i + 1));
            responsesJson_.emplace_back();
            auto status = google::protobuf::util::MessageToJsonString(responses_.back(), &responsesJson_.back());
            EXPECT_TRUE(status.ok()) << "Failed to convert test response to JSON";

            // Default expectations for test params_ for GET calls.
            EXPECT_CALL(dm0_, getParam(oids_[i], testing::_, testing::_)).WillRepeatedly(testing::Invoke(
                [this, i](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
                    // Make sure authz is correctly passed in.
                    EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    return std::move(params_[i]);
                }));
            EXPECT_CALL(*params_.back(), getOid()).WillRepeatedly(testing::ReturnRefOfCopy(oids_[i]));
            EXPECT_CALL(*params_.back(), toProto(testing::An<catena::Param&>(), testing::_)).WillRepeatedly(testing::Invoke(
                [this, i](catena::Param &param, catena::common::Authorizer &authz) {
                    // Make sure authz is correctly passed in.
                    EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    param.CopyFrom(responses_[i].param());
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));

            // Default expectations for test params_ for PUT calls.
            EXPECT_CALL(subManager_, removeSubscription(oids_[i], testing::_, testing::_)).WillRepeatedly(testing::Invoke(
                [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
                    // Make sure device is correctly passed in.
                    EXPECT_EQ(&dm, &dm0_);
                    this->removedOids_++;
                    return true;
                }));
            EXPECT_CALL(subManager_, addSubscription(oids_[i], testing::_, testing::_, testing::_)).WillRepeatedly(testing::Invoke(
                [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
                    // Make sure authz and device are correctly passed in.
                    EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
                    EXPECT_EQ(&dm, &dm0_);
                    this->addedOids_++;
                    return true;
                }));
        }
    }

    /*
     * Creates a Subscriptions handler object.
     */
    ICallData* makeOne() override { return Subscriptions::makeOne(serverSocket_, context_, dms_); }

    /*
     * Streamlines the creation of endpoint_ input. 
     */
    void initPayload(uint32_t slot, const std::vector<std::string> &addOids= {}, const std::vector<std::string> &remOids = {}) {
        slot_ = slot;
        // Adding addOids.
        for (const auto& oid : addOids) { inVal_.add_added_oids(oid); }
        // Adding remOids.
        for (const auto& oid : remOids) { inVal_.add_removed_oids(oid); }
        // Converting to JSON body.
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        ASSERT_TRUE(status.ok()) << "Failed to convert input value to JSON";
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::vector<std::string> jsonBodies;
        // Additional stuff based on method used.
        if (expRc_.status == catena::StatusCode::OK) {
            if (method_ == Method_GET) {
                for (auto param : responses_) {
                    jsonBodies.emplace_back();
                    auto status = google::protobuf::util::MessageToJsonString(param, &jsonBodies.back());
                    ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
                }
            } else if (method_ == Method_PUT) {
                EXPECT_EQ(addedOids_, inVal_.added_oids().size());
                EXPECT_EQ(removedOids_, inVal_.removed_oids().size());
            }
        }
        // Response format based on stream or unary.
        if (stream_) {
            EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
        } else {
            EXPECT_EQ(readResponse(), expectedResponse(expRc_, jsonBodies));
        }
    }

    // in vals
    catena::UpdateSubscriptionsPayload inVal_;
    // Test params_.
    std::vector<std::string> oids_{"param1", "param2"};
    std::vector<std::unique_ptr<MockParam>> params_;
    std::vector<catena::DeviceComponent_ComponentParam> responses_;
    std::vector<std::string> responsesJson_;
    // Trackers for calls to add/remove subscriptions.
    uint32_t addedOids_ = 0;
    uint32_t removedOids_ = 0;

    MockSubscriptionManager subManager_;
};

/* 
 * TEST 0.1 - Creating a Subscriptions object with makeOne.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_create) {
    ASSERT_TRUE(endpoint_);
}

/* 
 * TEST 0.2 - Writing to console with Subscriptions finish().
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("Subscriptions[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - Subscriptions with a device which does not support them.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_NotSupported) {
    initPayload(0);
    expRc_ = catena::exception_with_status("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);
    // Setting expectations.
    EXPECT_CALL(dm0_, subscriptions()).WillOnce(testing::Return(false));
    EXPECT_CALL(context_, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 0.4 - Subscriptions with an invalid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_AuthzInalid) {
    initPayload(0);
    expRc_ = catena::exception_with_status("", catena::StatusCode::UNAUTHENTICATED);
    authzEnabled_ = true;
    jwsToken_ = "Invalid token";
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    EXPECT_CALL(context_, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 0.5 - Subscriptions with an invalid method.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_BadMethod) {
    initPayload(0);
    expRc_ = catena::exception_with_status("Bad method", catena::StatusCode::UNIMPLEMENTED);
    method_ = Method_NONE;
    // Setting expectations.
    EXPECT_CALL(context_, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 0.6 - Subscriptions with an invalid slot.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_InvalidSlot) {
    initPayload(dms_.size());
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(slot_), catena::StatusCode::NOT_FOUND);
    // Setting expectations.
    EXPECT_CALL(context_, getSubscriptionManager()).Times(0); // Should not call.
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              GET Subscriptions tests
 * ============================================================================
 * 
 * TEST 1.1 - GET Subscriptions normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETNormal) {
    initPayload(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.2 - GET Stream normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETStream) {
    initPayload(0);
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.3 - GET Subscriptions with a valid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETAuthzValid) {
    initPayload(0);
    authzEnabled_ = true;
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.4 - GET Subscriptions fail to retrieve a param.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamReturnErr) {
    initPayload(0);
    oids_.insert(oids_.begin(), "errParam");
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            status = catena::exception_with_status("Param not found", catena::StatusCode::NOT_FOUND);
            return nullptr; // Simulating an error.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.5 - GET Subscriptions call to GetParam throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamThrowCatena) {
    initPayload(0);
    expRc_ = catena::exception_with_status{"Param not found", catena::StatusCode::NOT_FOUND};
    oids_.insert(oids_.begin(), "errParam");
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [this](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr; // Simulating an error.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.6 - GET Subscriptions call to GetParam throws an std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETGetParamThrowUnknown) {
    initPayload(0);
    expRc_ = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
    oids_.insert(oids_.begin(), "errParam");
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error("Unknown error occurred")));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.7 - GET Subscriptions fails to convert param to proto.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoReturnErr) {
    initPayload(0);
    oids_.insert(oids_.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(testing::ReturnRef(oids_[0]));
    EXPECT_CALL(*errParam, toProto(testing::An<catena::Param&>(), testing::_)).WillRepeatedly(testing::Invoke(
        [](catena::Param &param, catena::common::Authorizer &authz) {
            // Simulating an error in conversion.
            return catena::exception_with_status("Failed to convert to proto", catena::StatusCode::UNKNOWN);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.8 - GET Subscriptions call to toProto throws an catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoThrowCatena) {
    initPayload(0);
    expRc_ = catena::exception_with_status{"Param not found", catena::StatusCode::NOT_FOUND};
    oids_.insert(oids_.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();

    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(testing::ReturnRef(oids_[0]));
    EXPECT_CALL(*errParam, toProto(testing::An<catena::Param&>(), testing::_)).WillRepeatedly(testing::Invoke(
        [this](catena::Param &param, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.9 - GET Subscriptions call to toProto throws an std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_GETToProtoThrowUnknown) {
    initPayload(0);
    expRc_ = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
    oids_.insert(oids_.begin(), "errParam");
    std::unique_ptr<MockParam> errParam = std::make_unique<MockParam>();
    // Setting expectations.
    EXPECT_CALL(dm0_, getParam("errParam", testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [&errParam](const std::string& oid, catena::exception_with_status& status, catena::common::Authorizer& authz) {
            return std::move(errParam);
        }));
    EXPECT_CALL(*errParam, getOid()).WillRepeatedly(testing::ReturnRef(oids_[0]));
    EXPECT_CALL(*errParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillRepeatedly(testing::Throw(std::runtime_error("Unknown error")));
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              PUT Subscriptions tests
 * ============================================================================
 * 
 * TEST 2.1 - PUT Subscriptions add only
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAddOnly) {
    method_ = Method_PUT;
    initPayload(0, {"param1", "param2"}, {});  
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 2.2 - PUT Subscriptions remove only
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTRemoveOnly) {
    method_ = Method_PUT;
    initPayload(0, {}, {"param1", "param2"});
    // Calling proceed and testing the output
    testCall();
}

 /*
 * TEST 2.3 - PUT Subscriptions simultaneous add and remove.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTNormal) {
    method_ = Method_PUT;
    initPayload(0, {"param1", "param2"}, {"param1", "param2"});
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.4 - PUT Subscriptions with a valid token.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAuthzValid) {
    method_ = Method_PUT;
    initPayload(0, {"param1", "param2"}, {"param1", "param2"});
    authzEnabled_ = true;
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.5 - PUT Subscriptions normal case.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTFailParse) {
    method_ = Method_PUT;
    expRc_ = catena::exception_with_status("Failed to parse JSON Body", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody_ = "Not a JSON string";
    // Setting expectations.
    EXPECT_CALL(context_, jwsToken()).Times(0); // Authz false
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.6 - PUT Subscriptions add and remove return an error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTReturnErr) {
    method_ = Method_PUT;
    initPayload(0, {"errParam", "param1", "param2"}, {"errParam", "param1", "param2"});
    
    // Set up expectations for errParam specifically to increment counters
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            rc = catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
            this->removedOids_++; 
            return false;
        }));
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
            // Simulating an error in adding subscription.
            rc = catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
            this->addedOids_++;
            return false;
        }));
    
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.7 - PUT Subscriptions remove throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTRemThrowCatena) {
    method_ = Method_PUT;
    expRc_ = catena::exception_with_status{"Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT};
    initPayload(0, {}, {"errParam", "param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return false;
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.8 - PUT Subscriptions remove throws a std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTRemThrowUnknown) {
    method_ = Method_PUT;
    expRc_ = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
    initPayload(0, {}, {"errParam", "param1", "param2"});
    // Setting expectations.
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_))
        .WillRepeatedly(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.9 - PUT Subscriptions add throws a catena::exception_with_status.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAddThrowCatena) {
    method_ = Method_PUT;
    expRc_ = catena::exception_with_status{"Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT};
    initPayload(0, {"errParam", "param1", "param2"}, {});
    // Setting expectations.
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, catena::common::Authorizer& authz) {
            // Simulating an error in adding subscription.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return false;
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.10 - PUT Subscriptions add throws a std::runtime_error.
 */
TEST_F(RESTSubscriptionsTests, Subscriptions_PUTAddThrowUnknown) {
    method_ = Method_PUT;
    expRc_ = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
    initPayload(0, {"errParam", "param1", "param2"}, {});
    // Setting expectations.
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}
