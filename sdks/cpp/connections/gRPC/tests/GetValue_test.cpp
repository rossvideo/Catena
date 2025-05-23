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
 * @brief This file is for testing the GetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/22
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
#include "../../common/tests/CommonMockClasses.h"
#include "gRPCMockClasses.h"
#include "ServerHelper.h"

// gRPC
#include "controllers/GetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetValueTests : public ::testing::Test, public ServerHelper {
  protected:

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        // oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating the server.
        createServer();
        createClient();

        EXPECT_CALL(service, registerItem(::testing::_)).Times(1).WillOnce(::testing::Return());
        EXPECT_CALL(service, cq()).Times(2).WillRepeatedly(::testing::Return(cq.get()));

        // Creating getValue object.
        testCall = new GetValue(&service, dm, true);
    }

    void TearDown() override {
        // Restoring cout
        // std::cout.rdbuf(oldCout);

        // Cleaning up.
        shutdown();
    }

    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::string createMsg = "status: 0, ok: true";
};

/*
 * ============================================================================
 *                               GetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetValue object.
 */
TEST_F(gRPCGetValueTests, GetValue_create) {
    // Making sure getValue is created from the SetUp step.
    ASSERT_TRUE(testCall);
    // ASSERT_TRUE(MockConsole.str().find(createMsg) != std::string::npos);
}

TEST_F(gRPCGetValueTests, GetValue_proceed) {
    catena::GetValuePayload inVal;
    catena::Value outVal;
    catena::Value returnVal;
    returnVal.set_string_value("Test string");
    catena::exception_with_status rc("", catena::StatusCode::OK);
    grpc::ClientContext context;

    // new GetValue(service, dm, true);
    EXPECT_CALL(service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            asyncCall = cd; // Keep track of obj to avoid mem leak.
        }));
    EXPECT_CALL(service, cq()).Times(2).WillRepeatedly(::testing::Return(cq.get()));
    // kProcess();
    EXPECT_CALL(service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mtx));
    EXPECT_CALL(dm, getValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([returnVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            std::cout<<"dm.getValue() called"<<std::endl;
            value.CopyFrom(returnVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Return());

    std::thread cqThread([this](){
        // Waiting for request from completion queue before calling proceed.
        void* ignored_tag;
        bool ignored_ok;
        cq->Next(&ignored_tag, &ignored_ok);
        testCall->proceed(true);
        cq->Next(&ignored_tag, &ignored_ok);
        testCall->proceed(true);
    });

    stub->async()->GetValue(&context, &inVal, &outVal, [](grpc::Status status){ 
        std::cout<<"In the lambda function"<<std::endl;
    });

    cqThread.join();
    std::cout<<"AsyncGetValue() done." << std::endl;
}



