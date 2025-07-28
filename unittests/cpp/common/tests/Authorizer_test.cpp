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

class AuthorizationTest : public ::testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("AuthorizationTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    // Common test tokens
    const std::vector<std::pair<std::string, std::string>> testTokens = []() {
        // Add all scopes from the map (excluding kUndefined)
        std::vector<std::pair<std::string, std::string>> tokenPairs;
        for (const auto& [scopeEnum, scopeStr] : Scopes().getForwardMap()) {
            if (scopeEnum != Scopes_e::kUndefined) {
                tokenPairs.emplace_back(scopeStr, getJwsToken(scopeStr));
                tokenPairs.emplace_back(scopeStr + ":w", getJwsToken(scopeStr + ":w")); 
            }
        }
        return tokenPairs;
    }();
  
};



/*
 * ============================================================================
 *                               Authorizer tests
 * ============================================================================
 * 
 * TEST 1 - Creating an authorizer object with a valid JWS token.
 */
TEST_F(AuthorizationTest, Authz_createValid) {
    // Valid tokens.
    for (const auto& [currentScope, currentToken] : testTokens) {
        EXPECT_NO_THROW(Authorizer authz(currentToken));
    }
}

/* 
 * TEST 2 - Failing to create an authorizer object with a invalid JWS token.
 */
TEST_F(AuthorizationTest, Authz_createInvalid) {
    // Invalid token.
    std::string invalid_token = "This is not a valid token";
    EXPECT_THROW(Authorizer authz(invalid_token), catena::exception_with_status);
}

/* 
 * TEST 3 - Testing readAuthz().
 */
TEST_F(AuthorizationTest, Authz_readAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "readAuthz should be true when the authorizer have the scope.";
    std::string falseMsg = "readAuthz should be false when the authorizer does not have the scope.";
    // Testing with tokens of each scope.
    for (const auto& [currentScope, currentToken] : testTokens) {
        std::unique_ptr<Authorizer> authz = nullptr;
        EXPECT_NO_THROW(authz = std::make_unique<Authorizer>(currentToken));
        // Testing readAuthz for each scope.
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            // Authz has either scope or scope:w.
            if (currentScope.starts_with(scopeStr)) {
                EXPECT_TRUE(authz->readAuthz(scopeEnum))  << trueMsg;
                EXPECT_TRUE(authz->readAuthz(scopeStr))   << trueMsg;
                EXPECT_TRUE(authz->readAuthz(param))      << trueMsg;
                EXPECT_TRUE(authz->readAuthz(pd))         << trueMsg;
            // Authz does not have scope or scope:w.
            } else {
                EXPECT_FALSE(authz->readAuthz(scopeEnum)) << falseMsg;
                EXPECT_FALSE(authz->readAuthz(scopeStr))  << falseMsg;
                EXPECT_FALSE(authz->readAuthz(param))     << falseMsg;
                EXPECT_FALSE(authz->readAuthz(pd))        << falseMsg;
            }
        }
    }
}

/* 
 * TEST 4 - Testing writeAuthz().
 */
TEST_F(AuthorizationTest, Authz_writeAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "writeAuthz should be true when the authorizer has the scope.";
    std::string falseMsg = "writeAuthz should be false when the authorizer does not have the scope.";
    std::string rOnlyMsg = "writeAuthz should be false when the param is readOnly";
    // Testing with tokens of each scope.
    for (const auto& [currentScope, currentToken] : testTokens) {
        std::unique_ptr<Authorizer> authz = nullptr;
        EXPECT_NO_THROW(authz = std::make_unique<Authorizer>(currentToken));
        // Testing writeAuthz for each scope.
        for (bool rOnly : {false, true}) {
            for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
                EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                if (!rOnly) {
                    EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                    EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                }
                // Authz has either scope:w, depends on whether rOnly == true.
                if (scopeStr + ":w" == currentScope) {
                    EXPECT_TRUE(authz->writeAuthz(scopeEnum))   << trueMsg;
                    EXPECT_TRUE(authz->writeAuthz(scopeStr))    << trueMsg;
                    EXPECT_EQ(!rOnly, authz->writeAuthz(param)) << (rOnly ? rOnlyMsg: trueMsg);
                    EXPECT_EQ(!rOnly, authz->writeAuthz(pd))    << (rOnly ? rOnlyMsg: trueMsg);
                // Authz does not have scope:w.
                } else {
                    EXPECT_FALSE(authz->writeAuthz(scopeEnum))  << falseMsg;
                    EXPECT_FALSE(authz->writeAuthz(scopeStr))   << falseMsg;
                    EXPECT_FALSE(authz->writeAuthz(param))      << falseMsg;
                    EXPECT_FALSE(authz->writeAuthz(pd))         << falseMsg;
                }
            }
        }
    }
}

/* 
 * TEST 5 - Testing authorizer with no scope.
 */
TEST_F(AuthorizationTest, Authz_scopeNone) {
    std::string noScope = getJwsToken("");
    // Error message.
    std::string errMsg = "Authz should return false if the authorizer does not have the specified scope.";
    std::unique_ptr<Authorizer> authz = nullptr;
    EXPECT_NO_THROW(authz = std::make_unique<Authorizer>(noScope));
    MockParam param;
    MockParamDescriptor pd;
    // readAuthz() should always return false if client has no scopes.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_FALSE(authz->readAuthz(scopeEnum)) << errMsg;
        EXPECT_FALSE(authz->readAuthz(scopeStr))  << errMsg;
        EXPECT_FALSE(authz->readAuthz(param))     << errMsg;
        EXPECT_FALSE(authz->readAuthz(pd))        << errMsg;
    }
    // writeAuthz() should always return false if client has no scopes.
    for (bool rOnly : {false, true}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            if (!rOnly) {
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            }
            EXPECT_FALSE(authz->writeAuthz(scopeEnum)) << errMsg;
            EXPECT_FALSE(authz->writeAuthz(scopeStr))  << errMsg;
            EXPECT_FALSE(authz->writeAuthz(param))     << errMsg;
            EXPECT_FALSE(authz->writeAuthz(pd))        << errMsg;
        }
    }
}

/* 
 * TEST 6 - Testing kAuthzDisabled.
 */
TEST_F(AuthorizationTest, Authz_disabled) {
    Authorizer* authz = &Authorizer::kAuthzDisabled;
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "Authz should always return true if disabled.";
    std::string rOnlyMsg = "writeAuthz should be false when the param is readOnly";
    // readAuthz() should always return true.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_TRUE(authz->readAuthz(scopeStr)) << trueMsg;
        EXPECT_TRUE(authz->readAuthz(param))    << trueMsg;;
        EXPECT_TRUE(authz->readAuthz(pd))       << trueMsg;;
    }
    // writeAuthz() should return true if the param is not read only.
    for (bool rOnly : {false, true}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            if (!rOnly) {
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            }
            EXPECT_TRUE(authz->writeAuthz(scopeEnum))   << trueMsg;
            EXPECT_TRUE(authz->writeAuthz(scopeStr))    << trueMsg;
            EXPECT_EQ(!rOnly, authz->writeAuthz(param)) << (rOnly ? rOnlyMsg : trueMsg);
            EXPECT_EQ(!rOnly, authz->writeAuthz(pd))    << (rOnly ? rOnlyMsg : trueMsg);
        }
    }
}

/* 
 * TEST 7 - Testing authorizer with multiple scopes.
 */
TEST_F(AuthorizationTest, Authz_scopeMulti) {
    // This token has st2138:mon and st2138:op:w scopes.
    std::string multiScopes = getJwsToken("st2138:mon st2138:op:w");
    std::unique_ptr<Authorizer> authz = nullptr;
    EXPECT_NO_THROW(authz = std::make_unique<Authorizer>(multiScopes));
    MockParam param;
    MockParamDescriptor pd;
    // Error messages.
    std::string trueMsg  = "authz should be true when the authorizer has the scope.";
    std::string falseMsg = "authz should be false when the authorizer does not have the scope.";
    std::string rOnlyMsg = "writeAuthz should be false when the param is readOnly";
    // readAuthz() should return true if the scope is op or mon.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        if (scopeStr == Scopes().getForwardMap().at(Scopes_e::kMonitor) || scopeStr == Scopes().getForwardMap().at(Scopes_e::kOperate)) {
            EXPECT_TRUE(authz->readAuthz(param))     << trueMsg;
            EXPECT_TRUE(authz->readAuthz(pd))        << trueMsg;
            EXPECT_TRUE(authz->readAuthz(scopeStr))  << trueMsg;
            EXPECT_TRUE(authz->readAuthz(scopeEnum)) << trueMsg;
        } else {
            EXPECT_FALSE(authz->readAuthz(param))     << falseMsg;
            EXPECT_FALSE(authz->readAuthz(pd))        << falseMsg;
            EXPECT_FALSE(authz->readAuthz(scopeStr))  << falseMsg;
            EXPECT_FALSE(authz->readAuthz(scopeEnum)) << falseMsg;
        }
    }
    // writeAuthz() should return true if the scope is op and param is not read-only.
    for (bool rOnly : {false, true}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            if (!rOnly) {
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            }
            if (scopeStr == Scopes().getForwardMap().at(Scopes_e::kOperate)) {
                EXPECT_TRUE(authz->writeAuthz(scopeEnum))   << trueMsg;
                EXPECT_TRUE(authz->writeAuthz(scopeStr))    << trueMsg;
                EXPECT_EQ(!rOnly, authz->writeAuthz(param)) << (rOnly ? rOnlyMsg: trueMsg);
                EXPECT_EQ(!rOnly, authz->writeAuthz(pd))    << (rOnly ? rOnlyMsg: trueMsg);
            } else {
                EXPECT_FALSE(authz->writeAuthz(scopeEnum))  << falseMsg;
                EXPECT_FALSE(authz->writeAuthz(scopeStr))   << falseMsg;
                EXPECT_FALSE(authz->writeAuthz(param))      << falseMsg;
                EXPECT_FALSE(authz->writeAuthz(pd))         << falseMsg;
            }
        }
    }
}
