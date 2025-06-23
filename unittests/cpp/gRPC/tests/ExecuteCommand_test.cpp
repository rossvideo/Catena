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
#include "MockParam.h"
#include "MockCommandResponder.h"
#include "GRPCTest.h"

// gRPC
#include "controllers/ExecuteCommand.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCExecuteCommandTests : public GRPCTest {
  protected:
    /*
     * Creates an ExecuteCommand handler object.
     */
    void makeOne() override { new ExecuteCommand(&service, dms, true); }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    class StreamReader : public grpc::ClientReadReactor<catena::CommandResponse> {
      public:
        StreamReader(std::vector<catena::CommandResponse>* outVals, grpc::Status* outRc)
            : outVals_(outVals), outRc_(outRc) {}
        /*
         * This function makes an async RPC to the MockServer.
         */
        void MakeCall(catena::CatenaService::Stub* client, grpc::ClientContext* clientContext, const catena::ExecuteCommandPayload* inVal) {
            // Sending async RPC.
            client->async()->ExecuteCommand(clientContext, inVal, this);
            StartRead(&outVal_);
            StartCall();
        }
        /*
        * Triggers when a read is done and adds output to outVals_.
        */
        void OnReadDone(bool ok) override {
            if (ok) {
                outVals_->emplace_back(outVal_);
                StartRead(&outVal_);
            }
        }
        /*
         * Triggers when the RPC is finished and notifies Await().
         */
        void OnDone(const grpc::Status& status) override {
            *outRc_ = status;
            done_ = true;
            cv_.notify_one();
        }
        /*
         * Blocks until the RPC is finished. 
         */
        inline void Await() { cv_.wait(lock_, [this] { return done_; }); }
    
      private:
        // Pointers to the output variables.
        grpc::Status* outRc_;
        std::vector<catena::CommandResponse>* outVals_;

        catena::CommandResponse outVal_;
        bool done_ = false;
        std::condition_variable cv_;
        std::mutex cv_mtx_;
        std::unique_lock<std::mutex> lock_{cv_mtx_};
    };

    /*
     * Streamlines the creation of executeCommandPayloads. 
     */
    void initPayload(uint32_t slot, const std::string& oid, const std::string& value, bool respond = true) {
        inVal.set_slot(slot);
        inVal.set_oid(oid);
        inVal.mutable_value()->set_string_value(value);
        inVal.set_respond(respond);
        respond_ = respond;
    }

    /*
    * Adds a response to the expected values.
    */
    void expResponse(const std::string& stringVal) {
        expVals.push_back(catena::CommandResponse());
        expVals.back().mutable_response()->set_string_value(stringVal);
    }
    /* 
    * Adds an exception to the expected values.
    */
    void expException(const std::string& type, const std::string& details) {
        expVals.push_back(catena::CommandResponse());
        expVals.back().mutable_exception()->set_type(type);
        expVals.back().mutable_exception()->set_details(details);
    }
    /*
    * Adds a no_response to the expected values.
    */
    void expNoResponse() {
        expVals.push_back(catena::CommandResponse());
        expVals.back().mutable_no_response();
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        StreamReader streamReader(&outVals, &outRc);
        streamReader.MakeCall(client.get(), &clientContext, &inVal);
        streamReader.Await();
        // Comparing the results.
        if (respond_) {
            ASSERT_EQ(outVals.size(), expVals.size()) << "Output missing >= 1 CommandResponse";
            for (uint32_t i = 0; i < outVals.size(); i++) {
                EXPECT_EQ(outVals[i].SerializeAsString(), expVals[i].SerializeAsString());
            }
        } else {
            EXPECT_TRUE(outVals.empty()) << "Output should be empty when respond is false";
        }
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another ExecuteCommand handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::ExecuteCommandPayload inVal;
    std::vector<catena::CommandResponse> outVals;
    // Expected variables
    std::vector<catena::CommandResponse> expVals;
    bool respond_ = false;

    std::unique_ptr<MockParam> mockCommand = std::make_unique<MockParam>();
    std::unique_ptr<MockCommandResponder> mockResponder = std::make_unique<MockCommandResponder>();
};

/*
 * ============================================================================
 *                               ExecuteCommand tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ExecuteCommand object.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - ExecuteCommand returns three CommandResponse responses.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalResponse) {
    initPayload(0, "test_command", "test_value", true);
    expResponse("test_response_1");
    expResponse("test_response_2");
    expResponse("test_response_3");
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal.value().SerializeAsString());
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]))
        .WillOnce(::testing::Return(expVals[2]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - ExecuteCommand returns a CommandResponse no response.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalNoResponse) {
    initPayload(0, "test_command", "test_value", true);
    expNoResponse();
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - ExecuteCommand returns a CommandResponse exception.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_NormalException) {
    initPayload(0, "test_command", "test_value", true);
    expException("test_exception_type", "test_exception_details");
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 5 - ExecuteCommand returns no response (respond = false).
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_RespondFalse) {
    initPayload(0, "test_command", "test_value", false);
    expResponse("test_response_1");
    expResponse("test_response_2");
    expResponse("test_response_3");
    // Mocking kProcess functions
    EXPECT_CALL(dm0, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]))
        .WillOnce(::testing::Return(expVals[2]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - ExecuteCommand returns a CommandResponse no response with authz
 *          enabled.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzValid) {
    initPayload(0, "test_command", "test_value", true);
    expNoResponse();
    // Adding authorization mockToken metadata. This it a random RSA token
    authzEnabled = true;
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
    clientContext.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.string_value(), "test_value");
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - ExecuteCommand fails from invalid JWS token.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzInvalid) { 
    expRc = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - ExecuteCommand fails from JWS token not being found.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_AuthzJWSNotFound) {
    expRc = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 9 - No device in the specified slot.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ErrInvalidSlot) {
    initPayload(dms.size(), "test_command", "test_value", true);
    expRc = catena::exception_with_status("device not found in slot " + std::to_string(dms.size()), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 10 - getCommand does not find a command.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandReturnError) {
    expRc = catena::exception_with_status("Command not found", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(expRc.what(), expRc.status);
            return nullptr;
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 11 - getCommand throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandThrowCatena) {
    expRc = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return nullptr;
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 12 - getCommand throws an std::runtime error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetCommandThrowUnknown) {
    expRc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 13 - executeCommand returns a nullptr.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandReturnError) {
    expRc = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([](const catena::Value& value) {
            return nullptr;
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 14 - executeCommand throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowCatena) {
    expRc = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return nullptr;
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 15 - executeCommand returns an std::runtime_error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowUnknown) {
    expRc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 16 - getNext throws a catena::exception_with_status.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetNextThrowCatena) {
    expRc = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "test_command", "test_value", false);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Invoke([this]() {
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return catena::CommandResponse();
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 17 - getNext throws a std::runtime_error.
 */
TEST_F(gRPCExecuteCommandTests, ExecuteCommand_GetNextThrowUnknown) {
    expRc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "test_command", "test_value", false);
    // Setting expectations
    EXPECT_CALL(dm0, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(dm1, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}
