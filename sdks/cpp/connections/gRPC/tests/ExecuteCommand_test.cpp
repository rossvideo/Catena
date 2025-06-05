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
 * @brief This file is for testing the ExecuteCommand.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/03
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
#include "controllers/ExecuteCommand.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCExecuteCommandTests : public ::testing::Test {
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
    class TestRPC : public grpc::ClientReadReactor<catena::CommandResponse> {
        public:
            /*
             * Adds a response to the expected values.
             */
            void expResponse(const std::string& stringVal) {
                catena::CommandResponse res;
                res.mutable_response()->set_string_value(stringVal);
                expVals.push_back(res);
            }
            /* 
             * Adds an exception to the expected values.
             */
            void expException(const std::string& type, const std::string& details) {
                catena::CommandResponse res;
                res.mutable_exception()->set_type(type);
                res.mutable_exception()->set_details(details);
                expVals.push_back(res);
            }
            /*
             * Adds a no_response to the expected values.
             */
            void expNoResponse() {
                catena::CommandResponse res;
                res.mutable_no_response();
                expVals.push_back(res);
            }
            /*
             * This function makes an async RPC to the MockServer.
             */
            void TestCall(catena::CatenaService::Stub* client, const catena::exception_with_status& expRc, const catena::ExecuteCommandPayload& inVal, uint32_t expRead = 0) {
                client->async()->ExecuteCommand(&clientContext, &inVal, this);
                StartRead(&outVal);
                StartCall();

                // Waiting for the RPC to finish.
                cv.wait(lock, [this] { return done; });
                // Comparing the results.
                EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
                EXPECT_EQ(outRc.error_message(), expRc.what());
                EXPECT_EQ(read, expRead);
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

            grpc::ClientContext clientContext;
            std::vector<catena::CommandResponse> expVals;
            catena::CommandResponse outVal;
            grpc::Status outRc;
            uint32_t read = 0;

            bool done = false;
            std::condition_variable cv;
            std::mutex cv_mtx;
            std::unique_lock<std::mutex> lock{cv_mtx};
    };

    /*
     * Streamlines the creation of executeCommandPayloads. 
     */
    catena::ExecuteCommandPayload createPayload(const std::string& oid, const std::string& value, bool respond = true, bool proceed = true) {
        catena::ExecuteCommandPayload inVal;
        inVal.set_oid(oid);
        inVal.mutable_value()->set_string_value(value);
        inVal.set_respond(respond);
        inVal.set_proceed(proceed);
        return inVal;
    }

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

    std::unique_ptr<MockParam> mockCommand = std::make_unique<MockParam>();
    std::unique_ptr<MockCommandResponder> mockResponder = std::make_unique<MockCommandResponder>();
};

MockServer gRPCExecuteCommandTests::mockServer;

/*
 * ============================================================================
 *                               ExecuteCommand tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ExecuteCommand object.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_create) {
    // Creating deviceRequest object.
    new ExecuteCommand(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - ExecuteCommand returns three CommandResponse responses.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalResponse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expResponse("test_response_1");
    testRPC.expResponse("test_response_2");
    testRPC.expResponse("test_response_3");
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", true, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(3)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Return(testRPC.expVals[2]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal, 3);
}

/*
 * TEST 3 - ExecuteCommand returns a CommandResponse no response.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalNoResponse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expNoResponse();
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", true, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(testRPC.expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal, 1);
}

/*
 * TEST 4 - ExecuteCommand returns a CommandResponse exception.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalException) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expException("test_exception_type", "test_exception_details");
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", true, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(testRPC.expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal, 1);
}

/*
 * TEST 5 - ExecuteCommand returns no response (respond = false).
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_RespondFalse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    // We should not read these in the asyncReader.
    testRPC.expResponse("test_response_1");
    testRPC.expResponse("test_response_2");
    testRPC.expResponse("test_response_3");
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", false, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(3)
        .WillOnce(::testing::Return(testRPC.expVals[0]))
        .WillOnce(::testing::Return(testRPC.expVals[1]))
        .WillOnce(::testing::Return(testRPC.expVals[2]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal);
}

/*
 * TEST 6 - ExecuteCommand returns a CommandResponse no response with authz
 *          enabled.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expNoResponse();
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", true, true);
    // Adding authorization mockToken metadata. This it a random RSA token.
    std::string mockToken = "Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
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

    // Mocking kProcess functions
    mockServer.expectAuthz(&testRPC.clientContext, mockToken);
    EXPECT_CALL(*mockServer.dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_FALSE(&authz == &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(testRPC.expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal, 1);
}

/*
 * TEST 7 - ExecuteCommand fails from invalid JWS token.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzInvalid) { 
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);

    mockServer.expectAuthz(&testRPC.clientContext, "Bearer THIS SHOULD NOT PARSE");
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 8 - ExecuteCommand fails from JWS token not being found.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzJWSNotFound) {
    catena::exception_with_status rc("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);

    mockServer.expectAuthz(&testRPC.clientContext, "NOT A BEARER TOKEN");
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 9 - getCommand does not find a command.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandReturnError) {
    catena::exception_with_status rc("Command not found", catena::StatusCode::INVALID_ARGUMENT);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 10 - getCommand throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 11 - getCommand throws an std::runtime error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(rc.what())));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 12 - executeCommand returns a nullptr.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandReturnError) {
    catena::exception_with_status rc("Illegal state", catena::StatusCode::INTERNAL);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([](const catena::Value& value) {
            return nullptr;
        }));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 13 - executeCommand throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const catena::Value& value) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 14 - executeCommand returns an std::runtime_error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(rc.what())));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, catena::ExecuteCommandPayload());
}

/*
 * TEST 15 - getNext throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetNextThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", false, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Invoke([&rc]() {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::CommandResponse();
        }));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal);
}

/*
 * TEST 16 - getNext throws a std::runtime_error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetNextThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    catena::ExecuteCommandPayload inVal = createPayload("test_command", "test_value", false, true);

    // Mocking kProcess functions
    mockServer.expectAuthz();
    EXPECT_CALL(*mockServer.dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(rc.what())));
    mockServer.expectkFinish();

    testRPC.TestCall(mockServer.client.get(), rc, inVal);
}
