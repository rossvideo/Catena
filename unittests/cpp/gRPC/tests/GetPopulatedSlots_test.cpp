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
 * @brief This file is for testing the GetPopulatedSlots.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "controllers/GetPopulatedSlots.h"

// std
#include <thread>
#include <chrono>

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetPopulatedSlotsTests : public GRPCTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("gRPCGetPopulatedSlotsTest");
    }

    static void TearDownTestSuite() {
    }
  
    /*
     * Creates a GetPopulatedSlots handler object.
     */
    void makeOne() override { new GetPopulatedSlots(&service_, dms_, true); }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client_->async()->GetPopulatedSlots(&clientContext_, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        cv_.wait(lock_, [this] { return done_; });
        // Comparing the results.
        EXPECT_EQ(outVal_.SerializeAsString(), expVal_.SerializeAsString());
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another GetPopulatedSlots handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    double getRPCRequestStart(std::string expectedRequestStart) {
        done_ = false;
        double requestStart = -1.0;
        const int maxWaitMs = 5000;
        const auto startTime = std::chrono::steady_clock::now();

        // Create a new client context for this call to avoid metadata from previous calls
        grpc::ClientContext context;
        context.AddMetadata("request-start", expectedRequestStart);
        
        // Sending async RPC
        client_->async()->GetPopulatedSlots(&context, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        
        // Loop for checking handler
        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > maxWaitMs) {
                break; // Timeout waiting for RPC
            }

            auto* handler = dynamic_cast<GetPopulatedSlots*>(testCall_.get());
            if (handler != nullptr) {
                if (handler->getRequestReceieved_() != DEFAULT_REQUEST_RECEIVED) { // Check if processTimestamps_ has been called
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Give processTimestamps_ time to finish
                    requestStart = handler->getRequestStart_();
                    break;
                }
            }
        }
        cv_.wait(lock_, [this] { return done_; });
        
        // Clear the response for the next call
        outVal_.Clear();
        
        // Will return -1.0 if timed-out, 0.0 if invalid header value, or the parsed value
        return requestStart;
    }

    // in/out val
    st2138::Empty inVal_;
    st2138::SlotList outVal_;
    // Expected variables
    st2138::SlotList expVal_;
};

/*
 * ============================================================================
 *                               GetPopulatedSlots tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetPopulatedSlots object.
 */
TEST_F(gRPCGetPopulatedSlotsTests, GetPopulatedSlots_Create) {
    // Creating getPopulatedSlots object.
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for GetPopulatedSlots proceed().
 */
TEST_F(gRPCGetPopulatedSlotsTests, GetPopulatedSlots_Normal) {
    for (auto [slot, dm]: dms_) {
        expVal_.add_slots(slot);
    }
    // Sending the RPC and comparing the results.
    testRPC();
}

/*
 * TEST 3 - Test request-start header read correctly
 */
TEST_F(gRPCGetPopulatedSlotsTests, GetPopulatedSlots_RequestStart) {
    EXPECT_DOUBLE_EQ(getRPCRequestStart("12345.123"), 12345.123);
    EXPECT_DOUBLE_EQ(getRPCRequestStart("1.0"), 1.0);
    EXPECT_DOUBLE_EQ(getRPCRequestStart("123456789.12345"), 123456789.12345);
}

/*
 * TEST 3 - Test invalid request-start header value get set to default
 */
TEST_F(gRPCGetPopulatedSlotsTests, GetPopulatedSlots_InvalidRequestStart) {
    // Test with non-number/period in value
    EXPECT_DOUBLE_EQ(getRPCRequestStart("123@.123"), DEFAULT_REQUEST_START);
    // Test with multiple periods
    EXPECT_DOUBLE_EQ(getRPCRequestStart("123.123."), DEFAULT_REQUEST_START);
    // Test with negative value
    EXPECT_DOUBLE_EQ(getRPCRequestStart("-123.123"), DEFAULT_REQUEST_START);
    // Test with leading period
    EXPECT_DOUBLE_EQ(getRPCRequestStart(".123123"), DEFAULT_REQUEST_START);
    // Test with no period
    EXPECT_DOUBLE_EQ(getRPCRequestStart("123"), DEFAULT_REQUEST_START);
    // Test with too large of a value
    EXPECT_DOUBLE_EQ(getRPCRequestStart(std::string(309, '1')), DEFAULT_REQUEST_START);
}
