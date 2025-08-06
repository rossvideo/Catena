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
 * @brief This file is for testing the Authorizer.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "Authorizer.h"
#include "Enums.h"
#include "CommonTestHelpers.h"
#include <Logger.h>

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class TestAuthorizer : public Authorizer {
  public:
    TestAuthorizer(const std::string& jwsToken) : Authorizer(jwsToken) {}
    TestAuthorizer() : Authorizer() {}
    uint32_t exp() { return exp_; }
    void exp(uint32_t newExpiry) { exp_ = newExpiry; }
    ClientScopes& clientScopes() { return clientScopes_; }
    void clientScopes(const ClientScopes& newScopes) {
        clientScopes_ = newScopes;
    }
};

class AuthorizationTest : public ::testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("AuthorizationTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
};



/*
 * ============================================================================
 *                               Authorizer tests
 * ============================================================================
 * 
 * TEST 1 - Creating an authorizer object with a valid JWS token with scopes and exp.
 */
TEST_F(AuthorizationTest, CreateValid) {
    std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    // Creating the authorizer object.
    std::unique_ptr<TestAuthorizer> authz = nullptr;
    EXPECT_NO_THROW(authz = std::make_unique<TestAuthorizer>(getJwsToken("expired")));
    // Testing the extracted scopes and expiry.
    EXPECT_EQ(authz->clientScopes(), Authorizer::ClientScopes{Scopes().getForwardMap().at(Scopes_e::kMonitor)});
    EXPECT_EQ(authz->exp(), 1);
}
/* 
 * TEST 2 - Creating an authorizer object with a valid JWS token with no scopes or exp.
 */
TEST_F(AuthorizationTest, CreateNoFields) {
    std::string validToken = getJwsToken("");
    // Creating the authorizer object.
    std::unique_ptr<TestAuthorizer> authz = nullptr;
    EXPECT_NO_THROW(authz = std::make_unique<TestAuthorizer>(validToken));
    // Testing the extracted scopes and expiry.
    EXPECT_EQ(authz->clientScopes(), Authorizer::ClientScopes{});
    EXPECT_EQ(authz->exp(), 0);
}
/* 
 * TEST 3 - Failing to create an authorizer object with a invalid JWS token.
 */
TEST_F(AuthorizationTest, CreateInvalid) {
    // Invalid token.
    std::string invalid_token = "This is not a valid token";
    EXPECT_THROW(Authorizer authz(invalid_token), catena::exception_with_status);
}

/* 
 * TEST 4 - Testing readAuthz().
 */
TEST_F(AuthorizationTest, ReadAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "readAuthz should be true when the authorizer has the scope.";
    std::string falseMsg = "readAuthz should be false when the authorizer does not have the scope.";
    for (auto& [cScopeEnum, cScopeStr]: Scopes().getForwardMap()) {
        for (auto& suffix : {"", ":w"}) {
            TestAuthorizer authz;
            authz.clientScopes({cScopeStr + suffix});
            for (auto& [pScopeEnum, pScopeStr]: Scopes().getForwardMap()) {
                // Setting expectations for param and pd.
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
                // Testing results.
                bool hasAuthz = cScopeStr.starts_with(pScopeStr);
                EXPECT_EQ(hasAuthz, authz.readAuthz(pScopeEnum)) << (hasAuthz ? trueMsg : falseMsg);
                EXPECT_EQ(hasAuthz, authz.readAuthz(pScopeStr))  << (hasAuthz ? trueMsg : falseMsg);
                EXPECT_EQ(hasAuthz, authz.readAuthz(param))      << (hasAuthz ? trueMsg : falseMsg);
                EXPECT_EQ(hasAuthz, authz.readAuthz(pd))         << (hasAuthz ? trueMsg : falseMsg);
            }
        }
    }
}

/* 
 * TEST 5 - Testing writeAuthz().
 */
TEST_F(AuthorizationTest, WriteAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "writeAuthz should be true when the authorizer has the scope.";
    std::string falseMsg = "writeAuthz should be false when the authorizer does not have the scope.";
    std::string rOnlyMsg = "writeAuthz should be false when the param is readOnly";
    for (auto& [cScopeEnum, cScopeStr]: Scopes().getForwardMap()) {
        for (auto& suffix : {"", ":w"}) {
            TestAuthorizer authz;
            authz.clientScopes({cScopeStr + suffix});
            for (auto& [pScopeEnum, pScopeStr]: Scopes().getForwardMap()) {
                for (bool rOnly : {false, true}) {
                    // Setting expectations for param and pd.
                    EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                    EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                    if (!rOnly) {
                        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
                        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
                    }
                    // Testing results.
                    bool hasAuthz = cScopeStr + suffix == pScopeStr + ":w";
                    EXPECT_EQ(hasAuthz, authz.writeAuthz(pScopeEnum))      << (hasAuthz ? trueMsg : falseMsg);
                    EXPECT_EQ(hasAuthz, authz.writeAuthz(pScopeStr))       << (hasAuthz ? trueMsg : falseMsg);
                    EXPECT_EQ(!rOnly && hasAuthz, authz.writeAuthz(param)) << (rOnly ? rOnlyMsg : (hasAuthz ? trueMsg : falseMsg));
                    EXPECT_EQ(!rOnly && hasAuthz, authz.writeAuthz(pd))    << (rOnly ? rOnlyMsg : (hasAuthz ? trueMsg : falseMsg));
                }
            }
        }
    }
}

/* 
 * TEST 6 - Testing kAuthzDisabled.
 */
TEST_F(AuthorizationTest, kAuthzDisabled) {
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string falseMsg  = "Authz should always return true if disabled.";
    std::string rOnlyMsg = "writeAuthz should be false when the param is readOnly";
    for (auto& [pScopeEnum, pScopeStr]: Scopes().getForwardMap()) {
        // Setting expectations for param and pd.
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
        // Testing results.
        EXPECT_TRUE(Authorizer::kAuthzDisabled.readAuthz(pScopeEnum)) << falseMsg;
        EXPECT_TRUE(Authorizer::kAuthzDisabled.readAuthz(pScopeStr))  << falseMsg;
        EXPECT_TRUE(Authorizer::kAuthzDisabled.readAuthz(param))      << falseMsg;
        EXPECT_TRUE(Authorizer::kAuthzDisabled.readAuthz(pd))         << falseMsg;
    }
    // writeAuthz() should return true if the param is not read only.
    for (auto& [pScopeEnum, pScopeStr]: Scopes().getForwardMap()) {
        for (bool rOnly : {false, true}) {
            // Setting expectations for param and pd.
            EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            if (!rOnly) {
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(pScopeStr));
            }
            // Testing results.
            EXPECT_TRUE(Authorizer::kAuthzDisabled.writeAuthz(pScopeEnum))  << falseMsg;
            EXPECT_TRUE(Authorizer::kAuthzDisabled.writeAuthz(pScopeStr))   << falseMsg;
            EXPECT_EQ(!rOnly, Authorizer::kAuthzDisabled.writeAuthz(param)) << (rOnly ? rOnlyMsg : falseMsg);
            EXPECT_EQ(!rOnly, Authorizer::kAuthzDisabled.writeAuthz(pd))    << (rOnly ? rOnlyMsg : falseMsg);
        }
    }
}

/* 
 * TEST 7 - Testing isExpired.
 */
TEST_F(AuthorizationTest, isExpired) {
    TestAuthorizer authz{};
    EXPECT_FALSE(authz.isExpired()) << "Authz should not be expired if no exp is set.";
    authz.exp(1);
    EXPECT_TRUE(authz.isExpired())  << "Authz should be expired if exp is in the past.";
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    authz.exp(time + 100);
    EXPECT_FALSE(authz.isExpired()) << "Authz should not be expired exp is in the future.";
}
