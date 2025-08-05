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
 * @brief This file is for testing the GetParam.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// common
#include <Logger.h>

// Test helpers
#include "GRPCTest.h"
#include "CommonTestHelpers.h"
#include "MockParam.h"

// gRPC
#include "controllers/GetParam.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetParamTests : public GRPCTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCGetParamTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    /*
     * Creates a GetParam handler object.
     */
    void makeOne() override { new GetParam(&service_, dms_, true); }

    void initPayload(uint32_t slot, const std::string& oid) {
        inVal_.set_slot(slot);
        inVal_.set_oid(oid);
    }

    void initexpVal_(const std::string& oid, const std::string& value, const std::string& alias, const std::string& enName) {
        expVal_.set_oid(oid);
        auto param = expVal_.mutable_param();
        param->set_type(catena::ParamType::STRING);
        param->mutable_value()->set_string_value(value);
        param->add_oid_aliases(alias);
        (*param->mutable_name()->mutable_display_strings())["en"] = enName;
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client_->async()->GetParam(&clientContext_, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        cv_.wait(lock_, [this] { return done_; });
        // Comparing the results.
        EXPECT_EQ(outVal_.SerializeAsString(), expVal_.SerializeAsString());
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another GetParam handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // In/out val
    catena::GetParamPayload inVal_;
    catena::DeviceComponent_ComponentParam outVal_;
    // Expected variables
    catena::DeviceComponent_ComponentParam expVal_;

    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
};

/*
 * ============================================================================
 *                               GetParam tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetParam object.
 */
TEST_F(gRPCGetParamTests, GetParam_create) {
    // Creating getParam object.
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for GetParam proceed().
 */
TEST_F(gRPCGetParamTests, GetParam_Normal) {
    initPayload(0, "/test_oid");
    initexpVal_("/test_oid", "test_value", "test_alias", "Test Param");
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).WillRepeatedly(::testing::Invoke(
        [this](const std::string &fqoid, catena::exception_with_status &status, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockParam);
        }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal_.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [this](catena::Param &param, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            param.CopyFrom(expVal_.param());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 3 - GetParam with authz on and valid token.
 */
TEST_F(gRPCGetParamTests, GetParam_AuthzValid) {
    initPayload(0, "/test_oid");
    initexpVal_("/test_oid", "test_value", "test_alias", "Test Param");
    // Adding authorization mockToken metadata.
    authzEnabled_ = true;
    std::string mockToken = getJwsToken("st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w");
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [this](const std::string &fqoid, catena::exception_with_status &status, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockParam);
        }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal_.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [this](catena::Param &param, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            param.CopyFrom(expVal_.param());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 4 - GetParam with authz on and invalid token.
 */
TEST_F(gRPCGetParamTests, GetParam_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 5 - GetParam with authz on and invalid token.
 */
TEST_F(gRPCGetParamTests, GetParam_AuthzJWSNotFound) {
    expRc_ = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token.
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 6 - No device in the specified slot.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrInvalidSlot) {
    initPayload(dms_.size(), "/test_oid");
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 7 - dm.getParam() returns a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrGetParamReturnCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string &fqoid, catena::exception_with_status &status, const IAuthorizer &authz) {
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 8 - dm.getParam() throws a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrGetParamThrowCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string &fqoid, catena::exception_with_status &status, const IAuthorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 9 - dm.getParam() throws a std::runtime_exception.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrGetParamThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 10 - param->toProto() returns a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrToProtoReturnCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockParam); }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockParam, getOid()).WillRepeatedly(::testing::ReturnRef(expVal_.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Return(catena::exception_with_status(expRc_.what(), expRc_.status)));
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 11 - param->toProto() throws a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrToProtoThrowCatena) {
    expRc_ = catena::exception_with_status("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockParam); }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockParam, getOid()).WillRepeatedly(::testing::ReturnRef(expVal_.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::Param &param, const IAuthorizer &authz)  {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 12 - param->toProto() throws a std::runtime_exception.
 */
TEST_F(gRPCGetParamTests, GetParam_ErrToProtoThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "/test_oid");
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm0_, getParam(inVal_.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockParam); }));
    EXPECT_CALL(dm1_, getParam(::testing::An<const std::string&>(), ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockParam, getOid()).WillRepeatedly(::testing::ReturnRef(expVal_.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    // Sending the RPC.
    testRPC();
}
