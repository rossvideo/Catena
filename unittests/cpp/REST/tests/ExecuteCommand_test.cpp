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

// Test helpers
#include "RESTTest.h"
#include "MockCommandResponder.h"
#include "MockParam.h"

// REST
#include "controllers/ExecuteCommand.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTExecuteCommandTests : public RESTEndpointTest {
  protected:
    /*
     * Sets default expectations for hasField("respond").
     */
    RESTExecuteCommandTests() : RESTEndpointTest() {
        EXPECT_CALL(context_, hasField("respond")).WillRepeatedly(testing::Invoke([this]() { return respond_; }));
         // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, getCommand(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    }

    /*
     * Creates an ExecuteCommand handler object.
     */
    ICallData* makeOne() override { return ExecuteCommand::makeOne(serverSocket_, context_, dms_); }

    /*
     * Streamlines the creation of endpoint input. 
     */
    void initPayload(uint32_t slot, const std::string& oid, const std::string& value, bool respond = true) {
        slot_ = slot;
        fqoid_ = oid;
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        ASSERT_TRUE(status.ok()) << "Failed to convert Value to JSON";
        respond_ = respond;
    }

    /*
    * Adds a response to the expected values.
    */
    void expResponse(const std::string& stringVal) {
        expVals_.push_back(catena::CommandResponse());
        expVals_.back().mutable_response()->set_string_value(stringVal);
    }

    /* 
    * Adds an exception to the expected values.
    */
    void expException(const std::string& type, const std::string& details) {
        expVals_.push_back(catena::CommandResponse());
        expVals_.back().mutable_exception()->set_type(type);
        expVals_.back().mutable_exception()->set_details(details);
    }

    /*
    * Adds a no_response to the expected values.
    */
    void expNoResponse() {
        expVals_.push_back(catena::CommandResponse());
        expVals_.back().mutable_no_response();
    }
    
    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::vector<std::string> jsonBodies;
        if (respond_) {
            for (const auto& expVal : expVals_) {
                jsonBodies.push_back("");
                auto status = google::protobuf::util::MessageToJsonString(expVal, &jsonBodies.back());
                ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
            }
        }
        // Response format based on stream or unary.
        if (stream_) {
            EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
        } else {
            EXPECT_EQ(readResponse(), expectedResponse(expRc_, jsonBodies));
        }
    }

    // in variables
    catena::Value inVal_;
    bool respond_ = false;
    // Expected variables
    std::vector<catena::CommandResponse> expVals_;

    std::unique_ptr<MockParam> mockCommand_ = std::make_unique<MockParam>();
    std::unique_ptr<MockCommandResponder> mockResponder_ = std::make_unique<MockCommandResponder>();
};

/*
 * ============================================================================
 *                               ExecuteCommand tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ExecuteCommand object with makeOne.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_Create) {
    ASSERT_TRUE(endpoint_);
}

/*
 * TEST 2 - Writing to console with GetParam finish().
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("ExecuteCommand[1] finished\n") != std::string::npos);
}

/*
 * TEST 3 - ExecuteCommand returns two CommandResponse responses.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalResponse) {
    initPayload(0, "test_command", "test_value", true);
    expResponse("test_response_1");
    expResponse("test_response_2");
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(2)
        .WillOnce(testing::Return(expVals_[0]))
        .WillOnce(testing::Return(expVals_[1]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(3)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 4 - ExecuteCommand returns a CommandResponse no response.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalNoResponse) {
    initPayload(0, "test_command", "test_value", true);
    expNoResponse();
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1).WillOnce(testing::Return(expVals_[0]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 5 - ExecuteCommand returns a CommandResponse exception.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_NormalException) {
    initPayload(0, "test_command", "test_value", true);
    expException("test_exception_type", "test_exception_details");
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1).WillOnce(testing::Return(expVals_[0]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 6 - ExecuteCommand returns no response (respond = false).
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_RespondFalse) {
    initPayload(0, "test_command", "test_value", false);
    expResponse("test_response_1");
    expResponse("test_response_2");
    // Mocking kProcess functions
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(2)
        .WillOnce(testing::Return(expVals_[0]))
        .WillOnce(testing::Return(expVals_[1]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(3)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 7 - ExecuteCommand returns two CommandResponse responses.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_StreamResponse) {
    initPayload(0, "test_command", "test_value", true);
    expResponse("test_response_1");
    expResponse("test_response_2");
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(2)
        .WillOnce(testing::Return(expVals_[0]))
        .WillOnce(testing::Return(expVals_[1]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(3)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 8 - ExecuteCommand returns a CommandResponse no response.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_StreamNoResponse) {
    initPayload(0, "test_command", "test_value", true);
    expNoResponse();
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1).WillOnce(testing::Return(expVals_[0]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 9 - ExecuteCommand returns a CommandResponse exception.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_StreamException) {
    initPayload(0, "test_command", "test_value", true);
    expException("test_exception_type", "test_exception_details");
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1).WillOnce(testing::Return(expVals_[0]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 10 - ExecuteCommand returns no response (respond = false).
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_StreamRespondFalse) {
    initPayload(0, "test_command", "test_value", false);
    expResponse("test_response_1");
    expResponse("test_response_2");
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Mocking kProcess functions
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(2)
        .WillOnce(testing::Return(expVals_[0]))
        .WillOnce(testing::Return(expVals_[1]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(3)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 11 - ExecuteCommand returns a CommandResponse no response with authz
 *          enabled.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_AuthzValid) {
    initPayload(0, "test_command", "test_value", true);
    expNoResponse();
    // Adding authorization mockToken metadata. This it a random RSA token
    authzEnabled_ = true;
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(fqoid_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            // Making sure the correct values were passed in.
            EXPECT_EQ(value.SerializeAsString(), inVal_.SerializeAsString());
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1).WillOnce(testing::Return(expVals_[0]));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 12 - ExecuteCommand fails from invalid JWS token.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_AuthzInvalid) { 
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 13 - No device in the specified slot.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ErrInvalidSlot) {
    initPayload(dms_.size(), "test_command", "test_value", true);
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(slot_), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getCommand(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 14 - ExecuteCommand fails to parse the json body.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_InvalidJsonBody) {
    expRc_ = catena::exception_with_status("Failed to parse JSON body", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody_ = "THIS SHOULD NOT PARSE"; // Invalid JSON body.
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 15 - getCommand does not find a command.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandReturnError) {
    expRc_ = catena::exception_with_status("Command not found", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 16 - getCommand throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandThrowCatena) {
    expRc_ = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 17 - getCommand throws an std::runtime error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetCommandThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 18 - executeCommand returns a nullptr.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandReturnError) {
    expRc_ = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([](const catena::Value& value) {
            return nullptr;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 19 - executeCommand throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowCatena) {
    expRc_ = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 20 - executeCommand returns an std::runtime_error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_ExecuteCommandThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 21 - getNext throws a catena::exception_with_status.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetNextThrowCatena) {
    expRc_ = catena::exception_with_status("Threw error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "test_command", "test_value", false);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1)
        .WillOnce(testing::Invoke([this]() {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::CommandResponse();
        }));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(1).WillOnce(testing::Return(true));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 22 - getNext throws a std::runtime_error.
 */
TEST_F(RESTExecuteCommandTests, ExecuteCommand_GetNextThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "test_command", "test_value", false);
    // Setting expectations
    EXPECT_CALL(dm0_, getCommand(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string& oid, catena::exception_with_status& status, Authorizer& authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockCommand_);
        }));
    EXPECT_CALL(*mockCommand_, executeCommand(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value& value) {
            return std::move(mockResponder_);
        }));
    EXPECT_CALL(*mockResponder_, getNext()).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(*mockResponder_, hasMore()).Times(1).WillOnce(testing::Return(true));
    // Calling proceed and testing the output
    testCall();
}
