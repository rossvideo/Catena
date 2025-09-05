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
 * @brief This file is for testing the destructor for the ICallData Interface
 * @author nelson.daniels@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

/**
 * TO-DO
 * - Figure out to run tests
 * - Implement the use of GRPCTest.h
 * - Insure tests are what is WANTED to test the functionality of the destructor
 * - Improve Comments
 */

// Common
#include <string>

// Test helpers (Not currently used)
#include "GRPCTest.h"

// Using gtest until I can implement the GRPCTest functions
#include <gtest/gtest.h>

// gRPC
#include "interface/ICallData.h"

using namespace catena::common;
using namespace catena::gRPC;

// Tester class that inherits from ICallData interface
class gRPCICallDataTests : public ICallData {
    public:
        // Counters to track correct destruction of object
        static inline int alive = 0;
        static inline int destroyed = 0;

        // Handling constructor and destructor for object
        gRPCICallDataTests() { ++alive; }
        ~gRPCICallDataTests() override { ++destroyed; --alive; }

        // empty for purposes of testing
        void proceed(bool) override {}

    protected:
        std::string jwsToken_() const override {
            // Returns a constant for testing purposes
            return "test-token";
        } 
};

//----- Tests 

TEST(gRPCICallDataTests, DeleteThroughBasePointer) {
    gRPCICallDataTests::alive = 0;
    gRPCICallDataTests::destroyed = 0;

    ICallData* base = new gRPCICallDataTests();
    EXPECT_EQ(gRPCICallDataTests::alive, 1);

    delete base;
    EXPECT_EQ(gRPCICallDataTests::alive, 0);
    EXPECT_EQ(gRPCICallDataTests::destroyed, 1);
}

TEST(gRPCICallDataTests, UniquePtrBase_ScopeExitDestroysDerived) {
    gRPCICallDataTests::alive = 0;
    gRPCICallDataTests::destroyed = 0;

    {
        std::unique_ptr<ICallData> p = std::make_unique<gRPCICallDataTests>();
        EXPECT_EQ(gRPCICallDataTests::alive, 1);
    } // scope exit -> delete via ICallData*

    EXPECT_EQ(gRPCICallDataTests::alive, 0);
    EXPECT_EQ(gRPCICallDataTests::destroyed, 1);
}

TEST(gRPCICallDataTests, ContainerOfUniquePtrBase_DestroysAll) {
    gRPCICallDataTests::alive = 0;
    gRPCICallDataTests::destroyed = 0;

    {
        std::vector<std::unique_ptr<ICallData>> vec;
        vec.emplace_back(std::make_unique<gRPCICallDataTests>());
        vec.emplace_back(std::make_unique<gRPCICallDataTests>());
        EXPECT_EQ(gRPCICallDataTests::alive, 2);
    } // scope exit -> both destroyed via ICallData*

    EXPECT_EQ(gRPCICallDataTests::alive, 0);
    EXPECT_EQ(gRPCICallDataTests::destroyed, 2);
}
