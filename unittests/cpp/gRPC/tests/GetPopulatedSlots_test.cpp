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

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetPopulatedSlotsTests : public GRPCTest {
  protected:
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

    // in/out val
    catena::Empty inVal_;
    catena::SlotList outVal_;
    // Expected variables
    catena::SlotList expVal_;
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
