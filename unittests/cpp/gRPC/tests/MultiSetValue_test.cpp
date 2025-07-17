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
 * @brief This file is for testing the MultiSetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"
#include "CommonTestHelpers.h"

// gRPC
#include "controllers/MultiSetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCMultiSetValueTests : public GRPCTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCMultiSetValueTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    /*
     * Creates a MultiSetValue handler object.
     */
    void makeOne() override { new MultiSetValue(&service_, dms_, true); }

    /*
     * Helper function which initializes a MultiSetValuePayload object.
     */
    void initPayload(uint32_t slot, const std::vector<std::pair<std::string, std::string>>& setValues) {
        inVal_.set_slot(slot);
        for (const auto& [oid, value] : setValues) {
            auto addedVal = inVal_.mutable_values()->Add();
            addedVal->set_oid(oid);
            addedVal->mutable_value()->set_string_value(value);
        }
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client_->async()->MultiSetValue(&clientContext_, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        cv_.wait(lock_, [this] { return done_; });
        // Comparing the results.
        EXPECT_EQ(outVal_.SerializeAsString(), expVal_.SerializeAsString());
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another MultiSetValue handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::MultiSetValuePayload inVal_;
    catena::Empty outVal_;
    // Expected variables
    catena::Empty expVal_;
};

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_Create) {
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for MultiSetValue proceed().
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_Normal) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - MultiSetValue with authz on and valid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzValid) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    // Adding authorization mockToken metadata.
    authzEnabled_ = true;
    std::string mockToken = getJwsToken("st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w");
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - MultiSetValue with authz on and invalid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 5 - MultiSetValue with authz on and invalid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzJWSNotFound) {
    expRc_ = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token.
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - No device in the specified slot.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrInvalidSlot) {
    initPayload(dms_.size(), {});
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - dm.trySetValue returns a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryReturnCatena) {
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Setting the output status and returning false.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return false;
        }));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - dm.trySetValue throws a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryThrowCatena) {
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Throwing error and returning true.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 9 - dm.trySetValue throws a std::runtime_error.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryThrowUnknown) {
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 10 - dm.commitSetValue returns a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitReturnCatena) {
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Returning error status.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 11 - dm.commitSetValue throws a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitThrowCatena) {
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Throwing error and returning ok.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 12 - dm.commitSetValue throws a std::runtime_error.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitThrowUnknown) {
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}
