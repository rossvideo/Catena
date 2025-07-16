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
 * @brief This file is for testing the GetParam.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/21
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"
#include "MockParam.h"

// REST
#include "controllers/GetParam.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTGetParamTests : public RESTEndpointTest {
  protected:

    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTGetParamTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
    
    RESTGetParamTests() : RESTEndpointTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    }
    /*
     * Creates a GetParam handler object.
     */
    ICallData* makeOne() override { return GetParam::makeOne(serverSocket_, context_, dms_); }

    /*
     * Streamlines the creation of endpoint input. 
     */
    void initPayload(uint32_t slot, const std::string& oid) {
        slot_ = slot;
        fqoid_ = oid;
    }

    /*
     * Streamlines the creation of the expected output ComponentParam. 
     */
    void initExpVal(const std::string& oid, const std::string& value, const std::string& alias, const std::string& enName) {
        expVal_.set_oid(oid);
        auto param = expVal_.mutable_param();
        param->set_type(catena::ParamType::STRING);
        param->mutable_value()->set_string_value(value);
        param->add_oid_aliases(alias);
        (*param->mutable_name()->mutable_display_strings())["en"] = enName;
        auto status = google::protobuf::util::MessageToJsonString(expVal_, &expJson_);
        ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        EXPECT_EQ(readResponse(), expectedResponse(expRc_, expJson_));
    }

    // Expected variables
    catena::DeviceComponent_ComponentParam expVal_;
    std::string expJson_ = "";

    std::unique_ptr<MockParam> mockParam_ = std::make_unique<MockParam>();
};

/*
 * ============================================================================
 *                               GetParam tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetParam object.
 */
TEST_F(RESTGetParamTests, GetParam_Create) {
    EXPECT_TRUE(endpoint_);
}
/*
 * TEST 2 - Normal case for GetParam proceed().
 */
TEST_F(RESTGetParamTests, GetParam_Normal) {
    initPayload(0, "/test_oid");
    initExpVal("/test_oid", "test_value", "test_alias", "Test Param");
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockParam_);
        }));
    EXPECT_CALL(*mockParam_, getOid()).Times(1).WillOnce(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1).WillOnce(testing::Invoke(
        [this](catena::Param &param, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            param.CopyFrom(expVal_.param());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 3 - GetParam with authz on and valid token.
 */
TEST_F(RESTGetParamTests, GetParam_AuthzValid) {
    initPayload(0, "/test_oid");
    initExpVal("/test_oid", "test_value", "test_alias", "Test Param");
    // Adding authorization mockToken metadata. This it a random RSA token.
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
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1).WillOnce(testing::Invoke(
        [this](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockParam_);
        }));
    EXPECT_CALL(*mockParam_, getOid()).Times(1).WillOnce(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1).WillOnce(testing::Invoke(
        [this](catena::Param &param, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            param.CopyFrom(expVal_.param());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 4 - GetParam with authz on and invalid token.
 */
TEST_F(RESTGetParamTests, GetParam_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    jwsToken_ = "THIS SHOULD NOT PARSE";
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*mockParam_, getOid()).Times(0);
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 5 - No device in the specified slot.
 */
TEST_F(RESTGetParamTests, GetParam_ErrInvalidSlot) {
    initPayload(dms_.size(), "/test_oid");
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(slot_), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 6 - dm.getParam() returns a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_ErrGetParamReturnCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    EXPECT_CALL(*mockParam_, getOid()).Times(0);
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 7 - dm.getParam() throws a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_ErrGetParamThrowCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    EXPECT_CALL(*mockParam_, getOid()).Times(0);
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 8 - dm.getParam() throws a std::runtime_exception.
 */
TEST_F(RESTGetParamTests, GetParam_ErrGetParamThrowStd) {
    expRc_ = catena::exception_with_status("Std error", catena::StatusCode::INTERNAL);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(*mockParam_, getOid()).Times(0);
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 9 - dm.getParam() throws an unknown error.
 */
TEST_F(RESTGetParamTests, GetParam_ErrGetParamThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    EXPECT_CALL(*mockParam_, getOid()).Times(0);
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 10 - param->toProto() returns a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_ErrToProtoReturnCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](){ return std::move(mockParam_); }));
    EXPECT_CALL(*mockParam_, getOid()).WillRepeatedly(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Return(catena::exception_with_status(expRc_.what(), expRc_.status)));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 11 - param->toProto() throws a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_ErrToProtoThrowCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](){ return std::move(mockParam_); }));
    EXPECT_CALL(*mockParam_, getOid()).WillRepeatedly(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param &param, catena::common::Authorizer &authz)  {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 12 - param->toProto() throws a std::runtime_exception.
 */
TEST_F(RESTGetParamTests, GetParam_ErrToProtoThrowStd) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](){ return std::move(mockParam_); }));
    EXPECT_CALL(*mockParam_, getOid()).WillRepeatedly(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 13 - param->toProto() throws an unknown error.
 */
TEST_F(RESTGetParamTests, GetParam_ErrToProtoThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](){ return std::move(mockParam_); }));
    EXPECT_CALL(*mockParam_, getOid()).WillRepeatedly(testing::ReturnRef(fqoid_));
    EXPECT_CALL(*mockParam_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}
