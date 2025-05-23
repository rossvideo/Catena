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

// gRPC
#include "controllers/GetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetValueTests : public ::testing::Test {
  protected:

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        // oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating the server.
        std::cout<<"Creating server..."<<std::endl;
        builder.AddListeningPort(serverAddr, grpc::InsecureServerCredentials());
        cq = builder.AddCompletionQueue();
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        std::cout<<"Server created"<<std::endl;

        // Creating the client.
        std::cout<<"Creating client..."<<std::endl;
        channel = grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials());
        stub = catena::CatenaService::NewStub(channel);
        std::cout<<"Client created"<<std::endl;

        EXPECT_CALL(service, registerItem(::testing::_)).Times(1).WillOnce(::testing::Return());
        EXPECT_CALL(service, cq()).Times(2).WillRepeatedly(::testing::Return(cq.get()));

        // Creating getValue object.
        getValue = std::make_unique<GetValue>(&service, dm, true);
    }

    void TearDown() override {
        std::cout<<"Tearing down..."<<std::endl;
        // Restoring cout
        // std::cout.rdbuf(oldCout);
        // if (asyncObj) { delete asyncObj; }
        std::cout<<"asyncObj deleted"<<std::endl;
        if (server) { server->Shutdown(); }
        std::cout<<"server shutdown"<<std::endl;
        if (cq) { cq->Shutdown(); }
        std::cout<<"cq shutdown"<<std::endl;

        // // Cleaning up the server.
        // server->Shutdown();
        // server->Wait();
        // std::cout<<"server done wait"<<std::endl;
        // cq->Shutdown();
        // Draining the cq
        void* ignored_tag;
        bool ignored_ok;
        while (cq->Next(&ignored_tag, &ignored_ok)) {}
        std::cout<<"cq drained"<<std::endl;
    }

    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server = nullptr;
    MockServiceImpl service;
    std::unique_ptr<grpc::ServerCompletionQueue> cq = nullptr;
    std::shared_ptr<grpc::Channel> channel = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> stub = nullptr;

    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::mutex mtx;
    MockDevice dm;
    std::unique_ptr<ICallData> getValue = nullptr;
    ICallData* asyncObj = nullptr;

    std::string serverAddr = "0.0.0.0:50051";
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
    ASSERT_TRUE(getValue);
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
            asyncObj = cd; // Keep track of obj to avoid mem leak.
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
        void* ignored_tag;
        bool ignored_ok;
        std::cout<<"Calling cq->Next()..." << std::endl;
        cq->Next(&ignored_tag, &ignored_ok);
        std::cout<<"cq->Next() done." << std::endl;
        std::cout<<"Calling proceed()..." << std::endl;
        getValue->proceed(true);
        std::cout<<"Proceed() done." << std::endl;
        std::cout<<"Calling proceed()..." << std::endl;
        getValue->proceed(true);
        std::cout<<"Proceed() done." << std::endl;
    });

    std::cout<<"Calling AsyncGetValue()..." << std::endl;
    stub->async()->GetValue(&context, &inVal, &outVal, [](grpc::Status status){ 
        std::cout<<"In the lambda function"<<std::endl;
    });
    std::cout<<"AsyncGetValue() done." << std::endl;
    cqThread.join();

}

