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
 * @brief This file is for testing the AddLanguage.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/27
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "controllers/AddLanguage.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCAddLanguageTests : public GRPCTest {    
    public:
        gRPCAddLanguageTests() : GRPCTest() {
            inVal.set_slot(1);
            inVal.set_id("en");
            auto languagePack = inVal.mutable_language_pack();
            languagePack->set_name("English");
            (*languagePack->mutable_words())["greeting"] = "Hello";

            // Creating the AddLanguage object.
        }

    protected:

        /* 
        * Makes an async RPC to the MockServer and waits for a response before
        * comparing output.
        */
        void testRPC() {
            // Sending async RPC.
            mockServer.client->async()->AddLanguage(&clientContext, &inVal, &outVal, [this](grpc::Status status){
                outRc = status;
                done = true;
                cv.notify_one();
            });
            cv.wait(lock, [this] { return done; });
            // Comparing the results.
            EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
            EXPECT_EQ(outRc.error_code(), expRc.error_code());
            EXPECT_EQ(outRc.error_message(), expRc.error_message());
        }

        catena::AddLanguagePayload inVal;
        catena::Empty outVal;
};

/*
 * ============================================================================
 *                               AddLanguage tests
 * ============================================================================
 * 
 * TEST 1 - Creating a AddLanguage object.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_create) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for AddLanguage proceed().
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceeedNormal) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(language.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC and comparing the results.
    testRPC();
}

/*
 * TEST 3 - AddLanguage with authz on and valid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceeedAuthzValid) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    // Setting up the expected results.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
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
    clientContext.AddMetadata("authorization", "Bearer " + mockToken);

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(language.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_FALSE(&authz == &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC and comparing the results.
    testRPC();
}

/*
 * TEST 4 - AddLanguage with authz on and invalid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceeedAuthzInvalid) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    // Setting up the expected results.
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Not a token so it should get rejected by the authorizer.
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC and comparing the results.
    testRPC();
}

/*
 * TEST 5 - AddLanguage with authz on and invalid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceeedAuthzJWSNotFound) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Should not be able to find the bearer token.
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC and comparing the results.
    testRPC();
}

/*
 * TEST 6 - dm.addLanguage() returns a catena::exception_with_status.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceedErrReturnCatena) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("Language already exists", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 7 - dm.addLanguage() throws a catena::exception_with_status.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceedErrThrowCatena) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("Language already exists", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete this->mockServer.testCall;
        this->mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 8 - dm.addLanguage() throws a std::runtime_exception.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_proceedErrThrowUnknown) {
    new AddLanguage(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("unknown error", catena::StatusCode::UNKNOWN);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}
