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
 * @date 25/05/21
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
#include "SocketHelper.h"
#include "RESTMockClasses.h"
#include "../../common/tests/CommonMockClasses.h"

// REST
#include "controllers/ExecuteCommand.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTExecuteCommandTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTExecuteCommandTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating executeCommand object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        executeCommand = ExecuteCommand::makeOne(serverSocket, context, dm);
    }
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
     * Streamlines the creation of executeCommandPayloads. 
     */
    void setInVal(const std::string& oid, const std::string& value, bool respond = true) {
        inVal.set_oid(oid);
        inVal.mutable_value()->set_string_value(value);
        inVal.set_respond(respond);
        auto status = google::protobuf::util::MessageToJsonString(inVal.value(), &inValJsonBody);
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
     * A collection of expected calls to the context object across most tests.
     */
    void expContext() {
        EXPECT_CALL(context, hasField("respond")).Times(1).WillOnce(::testing::Return(inVal.respond()));
        if (inValJsonBody.empty()) {
            EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(inValJsonBody));
        } else {
            EXPECT_CALL(context, jsonBody()).Times(2).WillRepeatedly(::testing::ReturnRef(inValJsonBody));
        }
    }
    /*
     * A collection of expected calls for authorization across most tests.
     */
    void expAuthz(const std::string& mockToken = "") {
        EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(!mockToken.empty()));
        if (!mockToken.empty()) {
            EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
        }
    }
    /*
     * Calls proceed and tests the response.
     */
    void testCall(catena::exception_with_status& rc, bool respond = true) {
        executeCommand->proceed();
        std::vector<std::string> jsonBodies;
        if (respond) {
            for (const auto& expVal : expVals) {
                jsonBodies.push_back("");
                auto status = google::protobuf::util::MessageToJsonString(expVal, &jsonBodies.back());
            }
        }
        EXPECT_EQ(readResponse(), expectedSSEResponse(rc, jsonBodies));
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (executeCommand) {
            delete executeCommand;
        }
    }
    
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::vector<catena::CommandResponse> expVals;
    catena::ExecuteCommandPayload inVal;
    std::string inValJsonBody = "";
    
    MockSocketReader context;
    std::mutex mockMtx;
    MockDevice dm;
    catena::REST::ICallData* executeCommand = nullptr;

    std::unique_ptr<MockParam> mockCommand = std::make_unique<MockParam>();
    std::unique_ptr<MockCommandResponder> mockResponder = std::make_unique<MockCommandResponder>();
};

/*
 * ============================================================================
 *                               ExecuteCommand tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ExecuteCommand object with makeOne.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_create) {
    // Making sure executeCommand is created from the SetUp step.
    ASSERT_TRUE(executeCommand);
}

/*
 * TEST 2 - ExecuteCommand returns two CommandResponse responses.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalResponse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expResponse("test_response_1");
    expResponse("test_response_2");
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal.value().SerializeAsString());
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockResponder, getNext()).Times(2)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]));
    
    testCall(rc, inVal.respond());
}

/*
 * TEST 3 - ExecuteCommand returns a CommandResponse no response.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalNoResponse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expNoResponse();
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal.value().SerializeAsString());
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockResponder, hasMore()).Times(2)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));

    testCall(rc, inVal.respond());
}

/*
 * TEST 4 - ExecuteCommand returns a CommandResponse exception.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalException) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expException("test_exception_type", "test_exception_details");
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal.value().SerializeAsString());
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, hasMore()).Times(2)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    
    testCall(rc, inVal.respond());
}

/*
 * TEST 5 - ExecuteCommand returns no response (respond = false).
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_RespondFalse) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    // We should not read these in the asyncReader.
    expResponse("test_response_1");
    expResponse("test_response_2");
    setInVal("test_command", "test_value", false);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    // Mocking kWrite functions
    EXPECT_CALL(*mockResponder, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockResponder, getNext()).Times(2)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]));
    
    testCall(rc, inVal.respond());
}

/*
 * TEST 6 - ExecuteCommand returns a CommandResponse no response with authz
 *          enabled.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_AuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expNoResponse();
    setInVal("test_command", "test_value", true);
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

    // Mocking kProcess functions
    expContext();
    expAuthz(mockToken);
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
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
    EXPECT_CALL(*mockResponder, hasMore()).Times(2)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockResponder, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    
    testCall(rc, inVal.respond());
}

/*
 * TEST 7 - ExecuteCommand fails from invalid JWS token.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_AuthzInvalid) { 
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    std::string mockToken = "THIS SHOULD NOT PARSE";

    expContext();
    expAuthz(mockToken);

    testCall(rc);
}

/*
 * TEST 8 - ExecuteCommand fails to parse the json body.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_InvalidJsonBody) {
    catena::exception_with_status rc("Failed to parse JSON body", catena::StatusCode::INVALID_ARGUMENT);
    setInVal("test_command", "test_value", true);
    inValJsonBody = "THIS SHOULD NOT PARSE"; // Invalid JSON body.

    // Mocking kProcess functions
    expContext();

    testCall(rc);
}

/*
 * TEST 9 - getCommand does not find a command.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandReturnError) {
    catena::exception_with_status rc("Command not found", catena::StatusCode::INVALID_ARGUMENT);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));

    testCall(rc);
}

/*
 * TEST 10 - getCommand throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
        
    testCall(rc);
}

/*
 * TEST 11 - getCommand throws an std::runtime error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(rc.what())));
        
    testCall(rc);
}

/*
 * TEST 12 - executeCommand returns a nullptr.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandReturnError) {
    catena::exception_with_status rc("Illegal state", catena::StatusCode::INTERNAL);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1).WillOnce(::testing::Return(nullptr));
        
    testCall(rc);
}

/*
 * TEST 13 - executeCommand throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const catena::Value& value) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
        
    testCall(rc);
}

/*
 * TEST 14 - executeCommand returns an std::runtime_error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const catena::Value& value) {
            throw std::runtime_error(rc.what());
            return nullptr;
        }));
        
    testCall(rc);
}

/*
 * TEST 15 - getNext throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetNextThrowCatena) {
    catena::exception_with_status rc("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Invoke([&rc]() {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::CommandResponse();
        }));
        
    testCall(rc);
}

/*
 * TEST 16 - getNext throws a std::runtime_error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetNextThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    setInVal("test_command", "test_value", true);

    // Mocking kProcess functions
    expContext();
    expAuthz();
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(inVal.oid()));
    EXPECT_CALL(dm, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand);
        }));
    EXPECT_CALL(*mockCommand, executeCommand(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder);
        }));
    EXPECT_CALL(*mockResponder, hasMore()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockResponder, getNext()).Times(1)
        .WillOnce(::testing::Invoke([&rc]() {
            throw std::runtime_error(rc.what());
            return catena::CommandResponse();
        }));
        
    testCall(rc);
}

/*
 * TEST 17 - Writing to console with GetParam finish().
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_finish) {
    // Calling finish and expecting the console output.
    executeCommand->finish();
    ASSERT_TRUE(MockConsole.str().find("ExecuteCommand[16] finished\n") != std::string::npos);
}
