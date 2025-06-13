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
 * @brief This file is for testing the Authorization.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "Authorization.h"
#include "Enums.h"

using namespace catena::common;

// A collection of valid tokens with one scope each.
const std::unordered_map<std::string, std::string> testTokens {
    {Scopes().getForwardMap().at(Scopes_e::kMonitor),        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ"},
    {Scopes().getForwardMap().at(Scopes_e::kOperate),        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3AiLCJpYXQiOjE1MTYyMzkwMjJ9.lduNvr6tEaLFeIYR4bH5tC55WUSDBEe5PFz9rvGRD3o"},
    {Scopes().getForwardMap().at(Scopes_e::kConfig),         "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6Y2ZnIiwiaWF0IjoxNTE2MjM5MDIyfQ.n1dZJ01l8z4urxFUsSbUoaSJgflK828BHSLcxqTxOf4"},
    {Scopes().getForwardMap().at(Scopes_e::kAdmin),          "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6YWRtIiwiaWF0IjoxNTE2MjM5MDIyfQ.nqkypNl8hTMWC8zF1aIA_CvsfoOdbZrYpr9JN4T4sDs"},
    {Scopes().getForwardMap().at(Scopes_e::kMonitor) + ":w", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOnciLCJpYXQiOjE1MTYyMzkwMjJ9.QTHN7uqmk_jR2nVumyee3gMki-47tKOm_R0jnhT8Tpk"},
    {Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3A6dyIsImlhdCI6MTUxNjIzOTAyMn0.SNndYRi4apWLZfp-BoosQtmDDNFInVcMCMuh7djz-QI"},
    {Scopes().getForwardMap().at(Scopes_e::kConfig)  + ":w", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6Y2ZnOnciLCJpYXQiOjE1MTYyMzkwMjJ9.ty50rEHLJUlseD_6bj7KrmCm9NXVwHjbTAv1u392HCs"},
    {Scopes().getForwardMap().at(Scopes_e::kAdmin)   + ":w", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6YWRtOnciLCJpYXQiOjE1MTYyMzkwMjJ9.WrWmmNhw3EZ6AzZAytgZbvb_9NFL3_YtSSsZibW1P0w"}
};

/*
 * ============================================================================
 *                               Authorizer tests
 * ============================================================================
 * 
 * TEST 1 - Creating an authorizer object with a valid JWS token.
 */
TEST(AuthorizationTests, Authz_createValid) {
    // Valid tokens.
    for (auto& [currentScope, currentToken] : testTokens) {
        EXPECT_NO_THROW(Authorizer authz(currentToken));
    }
}

/* 
 * TEST 2 - Failing to create an authorizer object with a invalid JWS token.
 */
TEST(AuthorizationTests, Authz_createInvalid) {
    // Invalid token.
    std::string invalid_token = "This is not a valid token";
    EXPECT_THROW(Authorizer authz(invalid_token), catena::exception_with_status);
}

/* 
 * TEST 3 - Testing hasAuthz().
 */
TEST(AuthorizationTests, Authz_hasAuthz) {   
    for (auto& [currentScope, currentToken] : testTokens) {
        Authorizer* authz = nullptr;
        EXPECT_NO_THROW(authz = new Authorizer(currentToken));
        // Testing hasAuthz.
        for (std::string priviledge : {"", ":w"}) {
            for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
                if (scopeStr + priviledge == currentScope) {
                    EXPECT_TRUE(authz->hasAuthz(scopeStr + priviledge));
                } else {
                    EXPECT_FALSE(authz->hasAuthz(scopeStr + priviledge));
                }
            }
        }
        if (authz) { delete authz; }
    }
}

/* 
 * TEST 4 - Testing readAuthz().
 */
TEST(AuthorizationTests, Authz_readAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    for (auto& [currentScope, currentToken] : testTokens) {
        Authorizer* authz = nullptr;
        EXPECT_NO_THROW(authz = new Authorizer(currentToken));
        // Testing readAuthz(param).
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
            if (scopeStr == currentScope || scopeStr + ":w" == currentScope) {
                EXPECT_TRUE(authz->readAuthz(param));
                EXPECT_TRUE(authz->readAuthz(pd));
            } else {
                EXPECT_FALSE(authz->readAuthz(param));
                EXPECT_FALSE(authz->readAuthz(pd));
            }
        }
        if (authz) { delete authz; }
    }
}

/* 
 * TEST 5 - Testing writeAuthz().
 */
TEST(AuthorizationTests, Authz_writeAuthz) {
    MockParam param;
    MockParamDescriptor pd;
    for (auto& [currentScope, currentToken] : testTokens) {
        Authorizer* authz = nullptr;
        EXPECT_NO_THROW(authz = new Authorizer(currentToken));
        // Testing writeAuthz(param).
        for (bool rOnly : {false, true}) {
            for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
                EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
                if (!rOnly) {
                    EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                    EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                }
                if (scopeStr + ":w" == currentScope && !rOnly) {
                    EXPECT_TRUE(authz->writeAuthz(param));
                    EXPECT_TRUE(authz->writeAuthz(pd));
                } else {
                    EXPECT_FALSE(authz->writeAuthz(param));
                    EXPECT_FALSE(authz->writeAuthz(pd));
                }
            }
        }
        if (authz) { delete authz; }
    }
}

/* 
 * TEST 6 - Testing authorizer with no scope.
 */
TEST(AuthorizationTests, Authz_scopeNone) {
    std::string noScope = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMj"
                          "M0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2M"
                          "jM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw"
                          "5c";
    Authorizer* authz = nullptr;
    EXPECT_NO_THROW(authz = new Authorizer(noScope));
    MockParam param;
    MockParamDescriptor pd;
    // hasAuthz should always return false if client has no scopes.
    for (std::string priviledge : {"", ":w"}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_FALSE(authz->hasAuthz(scopeStr + priviledge));
        }
    }
    // readAuthz() should always return false if client has no scopes.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_FALSE(authz->readAuthz(param));
        EXPECT_FALSE(authz->readAuthz(pd));
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
            EXPECT_FALSE(authz->writeAuthz(param));
            EXPECT_FALSE(authz->writeAuthz(pd));
        }
    }
}

/* 
 * TEST 7 - Testing kAuthzDisabled.
 */
TEST(AuthorizationTests, Authz_disabled) {
    Authorizer* authz = &Authorizer::kAuthzDisabled;
    MockParam param;
    MockParamDescriptor pd;
    // hasAuthz should always return true.
    for (std::string priviledge : {"", ":w"}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_TRUE(authz->hasAuthz(scopeStr + priviledge));
        }
    }
    // readAuthz() should always return true.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_TRUE(authz->readAuthz(param));
        EXPECT_TRUE(authz->readAuthz(pd));
    }
    // writeAuthz() should return true if the param is not read only.
    for (bool rOnly : {false, true}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            EXPECT_CALL(param, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            EXPECT_CALL(pd, readOnly()).Times(1).WillOnce(::testing::Return(rOnly));
            if (rOnly) {
                EXPECT_FALSE(authz->writeAuthz(param));
                EXPECT_FALSE(authz->writeAuthz(pd));
            } else {
                EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
                EXPECT_TRUE(authz->writeAuthz(param));
                EXPECT_TRUE(authz->writeAuthz(pd));
            }
        }
    }
}

/* 
 * TEST 8 - Testing authorizer with multiple scopes.
 */
TEST(AuthorizationTests, Authz_scopeMulti) {
    // This token has st2138:mon and st2138:op:w scopes.
    std::string multiScopes = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOi"
                              "IxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2Nvc"
                              "GUiOiJzdDIxMzg6bW9uIHN0MjEzODpvcDp3IiwiaWF0Ijox"
                              "NTE2MjM5MDIyfQ.Z8upjHhZWKBlZ-yUcu7FFlJPby_C4jB9"
                              "Bnk-DGxoQyM";
    Authorizer* authz = nullptr;
    EXPECT_NO_THROW(authz = new Authorizer(multiScopes));
    MockParam param;
    MockParamDescriptor pd;
    // hasAuthz should return true if it has the correct scope.
    for (std::string priviledge : {"", ":w"}) {
        for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
            if (scopeStr + priviledge == Scopes().getForwardMap().at(Scopes_e::kMonitor)
                || scopeStr + priviledge == Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w") {
                EXPECT_TRUE(authz->hasAuthz(scopeStr + priviledge));
            } else {
                EXPECT_FALSE(authz->hasAuthz(scopeStr + priviledge));
            }
        }
    }
    // readAuthz() should return true if the scope is op or mon.
    for (auto& [scopeEnum, scopeStr]: Scopes().getForwardMap()) {
        EXPECT_CALL(param, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        EXPECT_CALL(pd, getScope()).Times(1).WillOnce(::testing::ReturnRef(scopeStr));
        if (scopeStr == Scopes().getForwardMap().at(Scopes_e::kMonitor)
            || scopeStr == Scopes().getForwardMap().at(Scopes_e::kOperate)) {
            EXPECT_TRUE(authz->readAuthz(param));
            EXPECT_TRUE(authz->readAuthz(pd));
        } else {
            EXPECT_FALSE(authz->readAuthz(param));
            EXPECT_FALSE(authz->readAuthz(pd));
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
            if (scopeStr == Scopes().getForwardMap().at(Scopes_e::kOperate) && !rOnly) {
                EXPECT_TRUE(authz->writeAuthz(param));
                EXPECT_TRUE(authz->writeAuthz(pd));
            } else {
                EXPECT_FALSE(authz->writeAuthz(param));
                EXPECT_FALSE(authz->writeAuthz(pd));
            }
        }
    }
}
