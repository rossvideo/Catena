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
 * @brief This file is for testing the DeviceRequest.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/26
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
#include "gRPCMockClasses.h"

// gRPC
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCDeviceRequestTests : public ::testing::Test {
  protected:
    /*
     * Called at the start of all tests.
     * Starts the mockServer and initializes the static inVal.
     */
    static void SetUpTestSuite() {
        mockServer.start();
    }

    /*
     * Sets up expectations for the creation of a new CallData obj.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // We can always assume that a new CallData obj is created.
        // Either from initialization or kProceed.
        mockServer.expNew();
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    class TestRPC : public grpc::ClientReadReactor<catena::DeviceComponent> {
        public:
            /*
             * This sets up the expected values for the RPC with one of
             * everything. They only have their OID set, which is enough to
             * make sure the correct obj is being passed back.
             */
            TestRPC() {
                expVals.push_back(catena::DeviceComponent()); // [0] Device
                expVals.push_back(catena::DeviceComponent()); // [1] Menu
                expVals.push_back(catena::DeviceComponent()); // [2] Language pack
                expVals.push_back(catena::DeviceComponent()); // [3] Constraint
                expVals.push_back(catena::DeviceComponent()); // [4] Param
                expVals.push_back(catena::DeviceComponent()); // [5] Comamnd
                expVals[0].mutable_device()->set_slot(1);
                expVals[1].mutable_menu()->set_oid("menu_test");
                expVals[2].mutable_language_pack()->set_language("language_test");
                expVals[3].mutable_shared_constraint()->set_oid("constraint_test");
                expVals[4].mutable_param()->set_oid("param_test");
                expVals[5].mutable_command()->set_oid("command_test");
            }
        
            /*
             * This function makes an async RPC to the MockServer.
             */
            void MakeCall(catena::CatenaService::Stub* client, const catena::DeviceRequestPayload& inVal) {
                client->async()->DeviceRequest(&clientContext, &inVal, this);
                StartRead(&outVal);
                StartCall();
            }
            /*
             * This function triggers when a read is done and compares the
             * returned outVal with the expected response.
             */
            void OnReadDone(bool ok) override {
                if (ok) {
                    // Test value.
                    EXPECT_EQ(outVal.SerializeAsString(), expVals[read].SerializeAsString());
                    read += 1;
                    StartRead(&outVal);
                }
            }
            /*
             * This function triggers when the RPC is finished and notifies
             * Await().
             */
            void OnDone(const grpc::Status& status) override {
                outRc = status;
                done = true;
                cv.notify_one();
            }
            /*
             * This function is used to wait for the RPC to finish and compares
             * the rc with what was expected.
             */
            void Await() {
                cv.wait(lock, [this] { return done; });
                // Comparing the results.
                EXPECT_EQ(outRc.error_code(), expRc.error_code());
                EXPECT_EQ(outRc.error_message(), expRc.error_message());
            }
        
            grpc::ClientContext clientContext;
            std::vector<catena::DeviceComponent> expVals;
            catena::DeviceComponent outVal;
            grpc::Status expRc;
            grpc::Status outRc;
            uint32_t read = 0;

            bool done = false;
            std::condition_variable cv;
            std::mutex cv_mtx;
            std::unique_lock<std::mutex> lock{cv_mtx};
    };

    /*
     * Restores cout after each test.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout);
    }

    /*
     * Called at the end of all tests, shuts down the server and cleans up.
     */
    static void TearDownTestSuite() {
        // Redirecting cout to a stringstream for testing.
        std::stringstream MockConsole;
        std::streambuf* oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // Destroying the server.
        EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
            delete mockServer.testCall;
            mockServer.testCall = nullptr;
        }));
        mockServer.shutdown();
        // Restoring cout
        std::cout.rdbuf(oldCout);
    }

    // Console variables
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    TestRPC testRPC;

    static MockServer mockServer;
};

MockServer gRPCDeviceRequestTests::mockServer;

/*
 * ============================================================================
 *                               DeviceRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a DeviceRequest object.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_create) {
    // Creating deviceRequest object.
    new DeviceRequest(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for DeviceRequest proceed().
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    catena::DeviceRequestPayload inVal;
    // No bearing just to make sure the currect val is being passed in.
    inVal.set_detail_level(catena::Device_DetailLevel::Device_DetailLevel_MINIMAL);
    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
    std::cerr <<"MockSerialer start of test: "<<mockSerializer<< std::endl;

    // Mocking kProcess functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer);
        }));
    //Mocking kWrite functions
    EXPECT_CALL(*mockServer.dm, mutex()).Times(6).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockSerializer, getNext()).Times(6)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Return(testRPC.expVals[2]))
        .WillOnce(::testing::Return(testRPC.expVals[3]))
        .WillOnce(::testing::Return(testRPC.expVals[4]))
        .WillOnce(::testing::Return(testRPC.expVals[5]));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(6)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    // Mocking kFinish functions.
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    testRPC.MakeCall(mockServer.client.get(), inVal);
    testRPC.Await();

    std::cerr <<"MockSerialer end of test: "<<mockSerializer<< std::endl;
    // std::cerr <<"MockSerialerPtr end of test: "<<mockSerializerPtr<< std::endl;
}

/*
 * TEST 3 - DeviceRequest proceed() with detail_level subscriptions.
 */
// TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedSubscriptions) {
//     catena::exception_with_status rc("", catena::StatusCode::OK);
//     testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
//     catena::DeviceRequestPayload inVal;
//     std::set<std::string> subscribedOids{"oid_test_1", "oid_test_2", "oid_test_3"};
//     inVal.set_detail_level(catena::Device_DetailLevel::Device_DetailLevel_SUBSCRIPTIONS);
//     inVal.add_subscribed_oids("oid_test_1");
//     inVal.add_subscribed_oids("oid_test_2");
//     inVal.add_subscribed_oids("oid_test_3");
//     std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
//     std::cerr <<"MockSerialer start of test: "<<mockSerializer<< std::endl;

//     // Mocking kProcess functions
//     EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
//         .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
//             // Making sure the correct values were passed in.
//             EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
//             EXPECT_TRUE(subscribedOids.contains("oid_test_1"));
//             EXPECT_TRUE(subscribedOids.contains("oid_test_2"));
//             EXPECT_TRUE(subscribedOids.contains("oid_test_3"));
//             return std::move(mockSerializer);
//         }));
//     //Mocking kWrite functions
//     EXPECT_CALL(*mockServer.dm, mutex()).Times(6).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
//     EXPECT_CALL(*mockSerializer, getNext()).Times(6)
//         .WillOnce(::testing::Return(testRPC.expVals[0]))
//         .WillOnce(::testing::Return(testRPC.expVals[1]))
//         .WillOnce(::testing::Return(testRPC.expVals[2]))
//         .WillOnce(::testing::Return(testRPC.expVals[3]))
//         .WillOnce(::testing::Return(testRPC.expVals[4]))
//         .WillOnce(::testing::Return(testRPC.expVals[5]));
//     EXPECT_CALL(*mockSerializer, hasMore()).Times(6)
//         .WillOnce(::testing::Return(true))
//         .WillOnce(::testing::Return(true))
//         .WillOnce(::testing::Return(true))
//         .WillOnce(::testing::Return(true))
//         .WillOnce(::testing::Return(true))
//         .WillOnce(::testing::Return(false));
//     // Mocking kFinish functions.
//     EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
//         delete mockServer.testCall;
//         mockServer.testCall = nullptr;
//     }));

//     testRPC.MakeCall(mockServer.client.get(), inVal);
//     testRPC.Await();

//     std::cerr <<"MockSerialer end of test: "<<mockSerializer<< std::endl;
//     // std::cerr <<"MockSerialerPtr end of test: "<<mockSerializerPtr<< std::endl;
// }

/*
 * TEST 4 - DeviceRequest with authz on and valid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    catena::DeviceRequestPayload inVal;
    // No bearing just to make sure the currect val is being passed in.
    inVal.set_detail_level(catena::Device_DetailLevel::Device_DetailLevel_MINIMAL);
    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
    // Adding authorization mockToken metadata. This it a random RSA token.
    std::string mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
                            "OiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2Nvc"
                            "GUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4Om"
                            "NmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMiw"
                            "ibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dTo"
                            "krEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko965"
                            "3v0khyUT4UKeOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKH"
                            "kWi4P3-CYWrwe-g6b4-a33Q0k6tSGI1hGf2bA9cRYr-VyQ_T3"
                            "RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEmuIwNOVM3EcVEaL"
                            "yISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg_w"
                            "bOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9M"
                            "dvJH-cx1s146M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    testRPC.clientContext.AddMetadata("authorization", "Bearer " + mockToken);
    std::cerr <<"MockSerialer start of test: "<<mockSerializer<< std::endl;

    // Mocking kProcess functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in.
            EXPECT_FALSE(&authz == &Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer);
        }));
    //Mocking kWrite functions
    EXPECT_CALL(*mockServer.dm, mutex()).Times(6).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockSerializer, getNext()).Times(6)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Return(testRPC.expVals[2]))
        .WillOnce(::testing::Return(testRPC.expVals[3]))
        .WillOnce(::testing::Return(testRPC.expVals[4]))
        .WillOnce(::testing::Return(testRPC.expVals[5]));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(6)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    // Mocking kFinish functions.
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    testRPC.MakeCall(mockServer.client.get(), inVal);
    testRPC.Await();

    std::cerr <<"MockSerialer end of test: "<<mockSerializer<< std::endl;
}

/*
 * TEST 5 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceeedAuthzInvalid) {
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Not a token so it should get rejected by the authorizer.
    testRPC.clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 6 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedAuthzJWSNotFound) {
    catena::exception_with_status rc("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Should not be able to find the bearer token.
    testRPC.clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 7 - dm.getComponentSerializer() returns nullptr.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedErrGetSerializerIllegalState) {
    catena::exception_with_status rc("Illegal state", catena::StatusCode::INTERNAL);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 8 - dm.getComponentSerializer() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedErrGetSerializerThrowCatena) {
    catena::exception_with_status rc("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 9 - dm.getComponentSerializer() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedErrGetSerializerThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            throw std::runtime_error(rc.what());
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 10 - serializer.getNext() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedErrGetNextThrowCatena) {
    catena::exception_with_status rc("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();

    // Mocking kProcess functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            return std::move(mockSerializer);
        }));
    //Mocking kWrite functions
    EXPECT_CALL(*mockServer.dm, mutex()).Times(3).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer, getNext()).Times(3)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Invoke([this, &rc](){
            throw catena::exception_with_status(rc.what(), rc.status);
            return testRPC.expVals[2];
        }));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(2)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true));
    // Mocking kFinish functions.
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}

/*
 * TEST 11 - serializer.getNext() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedErrGetNextThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();

    // Mocking kProcess functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            return std::move(mockSerializer);
        }));
    //Mocking kWrite functions
    EXPECT_CALL(*mockServer.dm, mutex()).Times(3).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer, getNext()).Times(3)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Invoke([this, &rc](){
            throw std::runtime_error(rc.what());
            return testRPC.expVals[2];
        }));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(2)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true));
    // Mocking kFinish functions.
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    testRPC.MakeCall(mockServer.client.get(), catena::DeviceRequestPayload());
    testRPC.Await();
}
