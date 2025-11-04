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
 * @file ServiceCredentials_test.cpp
 * @brief This file is for testing the gRPC ServiceCredentials.cpp file.
 * @author Jason Chen (jason.chen@rossvideo.com)
 * @date 2025-11-03
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"
#include "CommonTestHelpers.h"

// gRPC
#include <include/ServiceCredentials.h>

using namespace catena::common;
using namespace catena::gRPC;

class gRPCServiceCredentialsTests : public GRPCTest {
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("gRPCServiceCredentialsTest");
    }

    static void TearDownTestSuite() {
    }

    gRPCServiceCredentialsTests() : GRPCTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
        EXPECT_CALL(dm1_, getTopLevelParams(testing::_, testing::_)).Times(0);
    }

    /*
     * Creates a ParamInfoRequest handler object.
     */
    void makeOne() override { new ServiceCredentials(&service_, dms_, true); }

    void initPayload(uint32_t slot, const std::string& oid_prefix = "", bool recursive = false) {
        inVal_.set_slot(slot);
        inVal_.set_oid_prefix(oid_prefix);
        inVal_.set_recursive(recursive);
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and returns the streamed-back response.
     */
    using StreamReader = catena::gRPC::test::StreamReader<st2138::ParamInfoResponse, st2138::ParamInfoRequestPayload, 
        std::function<void(grpc::ClientContext*, const st2138::ParamInfoRequestPayload*, grpc::ClientReadReactor<st2138::ParamInfoResponse>*)>>;

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Creating the stream reader.
        StreamReader reader(&outVals_, &outRc_);
        // Making the RPC call.
        reader.MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->ParamInfoRequest(ctx, payload, reactor);
        });
        // Waiting for the RPC to finish.
        reader.Await();
        // Comparing the results.
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another ParamInfoRequest handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
        
        // Validate response content if we have expected responses
        if (!expVals_.empty()) {
            ASSERT_EQ(outVals_.size(), expVals_.size()) << "Expected " << expVals_.size() << " responses, got " << outVals_.size();
            for (size_t i = 0; i < outVals_.size(); i++) {
                EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString()) 
                    << "Response " << i << " does not match expected";
            }
        }
    }

    // In/out val
    st2138::ParamInfoRequestPayload inVal_;
    std::vector<st2138::ParamInfoResponse> outVals_;
    std::vector<st2138::ParamInfoResponse> expVals_;
};

// == SECTION 0: Preliminary tests ==

// 0.0 Preliminary test - Creation of ServiceCredentials handler
TEST_F(gRPCServiceCredentialsTests, ServiceCredentials_Create) {
    ASSERT_TRUE(asyncCall_);
}

