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
 * @brief This file is for testing the SetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
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
#include "GRPCTest.h"

// gRPC
#include "controllers/SetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCSetValueTests : public GRPCTest {
  protected:
    /*
     * Creates a SetValue handler object.
     */
    void makeOne() override { new SetValue(&service, dm, true); }

    /*
     * Helper function which initializes a SetValuePayload object and a MultiSetValuePayload object.
     */
    void initPayload(uint32_t slot, const std::string& oid, const std::string& value) {
        inVal.set_slot(1);
        auto val = inVal.mutable_value();
        val->set_oid(oid);
        val->mutable_value()->set_string_value(value);
        // Converting above to multiSetValuePayload for input testing.
        expMultiVal.set_slot(inVal.slot());
        expMultiVal.add_values()->CopyFrom(*val);
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client->async()->SetValue(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
        // Comparing the results.
        EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another SetValue handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::SingleSetValuePayload inVal;
    catena::Empty outVal;
    // Expected variables
    catena::Empty expVal;
    catena::MultiSetValuePayload expMultiVal;
};

/*
 * ============================================================================
 *                               SetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a SetValue object. This tests request_().
 */
TEST_F(gRPCSetValueTests, SetValue_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - Normal case for SetValue proceed().
 * This tests both create_() and toMulti_().
 */
TEST_F(gRPCSetValueTests, SetValue_Normal) {
    initPayload(0, "/test_oid", "test_value");
    // Setting expectations.
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            EXPECT_EQ(src.SerializeAsString(), expMultiVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            return true;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), expMultiVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC.
    testRPC();
}
