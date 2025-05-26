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
// #include "ServerHelper.h"

// gRPC
#include "controllers/GetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Server stuff
class MockServer {
  public:
    MockServer() {
        // Creating the mock server.
        builder.AddListeningPort(serverAddr, grpc::InsecureServerCredentials());
        cq = builder.AddCompletionQueue();
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        std::cout<<"Server created"<<std::endl;

        // Creating a client.
        channel = grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials());
        client = catena::CatenaService::NewStub(channel);

        // Deploying cq handler on a thread.
        cqthread = std::make_unique<std::thread>([&]() {
            void* ignored_tag;
            bool ignored_ok;
            while (cq->Next(&ignored_tag, &ignored_ok)) {
                std::cout << "Processing cq event" << std::endl;
                if (!testCall) {
                    testCall = asyncCall;
                    asyncCall = nullptr;
                }
                testCall->proceed(ok);
            }
        });
    }

    ~MockServer() {
        ok = false;
        // Cleaning up the server.
        server->Shutdown();
        // Cleaning the cq
        cq->Shutdown();
        cqthread->join();
        // Make sure the calldata objects were destroyed.
        EXPECT_TRUE(!testCall);
        EXPECT_TRUE(!asyncCall);
    }

    std::string serverAddr = "0.0.0.0:50051";
    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server = nullptr;
    MockServiceImpl service;
    std::mutex mtx;
    MockDevice dm;
    std::unique_ptr<grpc::ServerCompletionQueue> cq = nullptr;
    std::unique_ptr<std::thread> cqthread = nullptr;
    ICallData* testCall = nullptr;
    ICallData* asyncCall = nullptr;
    std::shared_ptr<grpc::Channel> channel = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> client = nullptr;
    bool ok = true;
};

MockServer* globalServer = nullptr;

// Fixture
class gRPCGetValueTests : public ::testing::Test {
  protected:

    void SetUp() override {
        
    }

    void TearDown() override {
        
    }


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
    globalServer = new MockServer();
    // Creating getValue object.
    EXPECT_CALL(globalServer->service, registerItem(::testing::_)).Times(1).WillOnce(::testing::Return());
    EXPECT_CALL(globalServer->service, cq()).Times(2).WillRepeatedly(::testing::Return(globalServer->cq.get()));

    // Creating getValue object.
    globalServer->testCall = new GetValue(&globalServer->service, globalServer->dm, true);

}

TEST_F(gRPCGetValueTests, GetValue_proceed) {
    // Setting up RPC input value.
    catena::GetValuePayload inVal;
    inVal.set_oid("/test_oid");
    // Setting up one var as expected RPC output and one to recieve RPC output.
    catena::Value expVal;
    expVal.set_string_value("Test string");
    catena::Value outVal;
    // Setting up an rcs.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    grpc::Status expRc(static_cast<grpc::StatusCode>(rc.status), rc.what());
    grpc::Status outRc;
    // Client stuff.
    grpc::ClientContext clientContext;
    bool done = false;
    std::condition_variable cv;
    std::mutex cv_mtx;
    std::unique_lock<std::mutex> lock(cv_mtx);

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(globalServer->service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            globalServer->asyncCall = cd;
        }));
    EXPECT_CALL(globalServer->service, cq()).Times(2).WillRepeatedly(::testing::Return(globalServer->cq.get()));
    // kProcess();
    EXPECT_CALL(globalServer->service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(globalServer->dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(globalServer->mtx));
    EXPECT_CALL(globalServer->dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([expVal, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            std::cout<<"dm.getValue() called"<<std::endl;
            value.CopyFrom(expVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(globalServer->service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete globalServer->testCall;
        globalServer->testCall = nullptr;
        std::cout<<"Deregistered testCall"<<std::endl;
    }));

    // Sending the RPC.
    globalServer->client->async()->GetValue(&clientContext, &inVal, &outVal, [&cv, &done, &outRc](grpc::Status status){ 
        std::cout<<"In the lambda function"<<std::endl;
        outRc = status;
        done = true;
        cv.notify_one();
    });
    std::cout<<"Waiting for the lambda to finish"<<std::endl;
    cv.wait(lock, [&done] { return done; });

    // Comparing output.
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
}

TEST_F(gRPCGetValueTests, GetValue_shutdown) {
    EXPECT_CALL(globalServer->service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete globalServer->testCall;
        globalServer->testCall = nullptr;
        std::cout<<"Deregistered testCall"<<std::endl;
    }));
    delete globalServer;
}
