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
 * @brief This file is for testing the LanguagePack.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/12
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"

// REST
#include "controllers/LanguagePack.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTLanguagePackTests : public RESTEndpointTest {
  protected:
    /*
     * Creates a LanguagePack handler object.
     */
    ICallData* makeOne() override { return LanguagePack::makeOne(serverSocket_, context_, dm0_); }

    /*
     * Streamlines the creation of endpoint input. 
     */
    void initPayload(uint32_t slot, const std::string& language) {
        slot_ = slot;
        language_ = language;
        fqoid_ = "/" + language;
    }
    /*
     * Streamlines the creation of endpoint input with json body. 
     */
    void initPayload(uint32_t slot, const std::string& language, const std::string& name, 
                     const std::unordered_map<std::string, std::string>& words) {
        initPayload(slot, language);
        inVal_.set_name(name);
        for (const auto& word : words) {
            inVal_.mutable_words()->insert({word.first, word.second});
        }
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
    }

    /*
     * Streamlines the creation of the expected output ComponentLanguagePack. 
     */
    void initExpVal(const std::string& language, const std::string& name, 
                    const std::unordered_map<std::string, std::string>& words) {
        expVal_.set_language(language);
        catena::LanguagePack *pack = expVal_.mutable_language_pack();
        pack->set_name(name);
        for (const auto& word : words) {
            pack->mutable_words()->insert({word.first, word.second});
        }
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::string expJson = "";
        if (!expVal_.language().empty()) {
            auto status = google::protobuf::util::MessageToJsonString(expVal_, &expJson);
            ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
        }
        EXPECT_EQ(readResponse(), expectedResponse(expRc_, expJson));
    }

    // In vals
    std::string language_; // fqoid_ without the leading "/"
    catena::LanguagePack inVal_;
    // Expected values
    catena::DeviceComponent_ComponentLanguagePack expVal_;
};

/*
 * TEST 0.1 - Creating a LanguagePack object with makeOne.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Create) {
    ASSERT_TRUE(endpoint_);
}

/* 
 * TEST 0.2 - Writing to console with LanguagePack finish().
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("LanguagePack[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - LanguagePack proceed() with an invalid method.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_BadMethod) {
    expRc_ = catena::exception_with_status("Bad method", catena::StatusCode::UNIMPLEMENTED);
    initPayload(0, "tl");
    method_ = Method_NONE;
    // Setting expectations.
    EXPECT_CALL(dm0_, getLanguagePack(testing::_, testing::_)).Times(0);
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    EXPECT_CALL(dm0_, removeLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              GET LanguagePack tests
 * ============================================================================
 * 
 * TEST 1.1 - GET LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETNormal) {
    initPayload(0, "tl");
    initExpVal("tl", "Test Language", {{"hello", "world"}});
    // Setting expectations.
    EXPECT_CALL(context_, authorizationEnabled()).Times(0); // No authz on GET.
    EXPECT_CALL(context_, jwsToken()).Times(0);             // No authz on GET.
    EXPECT_CALL(dm0_, getLanguagePack(language_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            pack.CopyFrom(expVal_);
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.2 - GET LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrReturn) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    // Setting expectations
    EXPECT_CALL(dm0_, getLanguagePack(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.3 - GET LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowCat) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    // Setting expectations
    EXPECT_CALL(dm0_, getLanguagePack(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.4 - GET LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowStd) {
    expRc_ = catena::exception_with_status("std error", catena::StatusCode::INTERNAL);
    initPayload(0, "tl");
    // Setting expectations
    EXPECT_CALL(dm0_, getLanguagePack(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.5 - GET LanguagePack dm.getLanguagePack() throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "tl");
    // Setting expectations
    EXPECT_CALL(dm0_, getLanguagePack(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              POST LanguagePack tests
 * ============================================================================
 * 
 * TEST 2.1 - POST LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTNormal) {
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), language_);
            EXPECT_EQ(language.language_pack().SerializeAsString(), inVal_.SerializeAsString());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.2 - POST LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTAuthzValid) {
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
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
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), language_);
            EXPECT_EQ(language.language_pack().SerializeAsString(), inVal_.SerializeAsString());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.3 - POST LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTAuthzInvalid) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::UNAUTHENTICATED);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(0);
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.4 - POST LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTFailParse) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    method_ = Method_POST;
    jsonBody_ = "Not a JSON string";
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(0);
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.5 - POST LanguagePack overwrite error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTOverwrite) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::PERMISSION_DENIED);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).WillOnce(testing::Return(true));
    // Should not call if trying to overwrite on POST.
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.6 - POST LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrReturn) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.7 - POST LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowCat) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.8 - POST LanguagePack throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowStd) {
    expRc_ = catena::exception_with_status("std error", catena::StatusCode::INTERNAL);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 2.9 - POST LanguagePack throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_POST;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              PUT LanguagePack tests
 * ============================================================================
 * 
 * TEST 3.1 - PUT LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTNormal) {
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), language_);
            EXPECT_EQ(language.language_pack().SerializeAsString(), inVal_.SerializeAsString());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTAuthzValid) {
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
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
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), language_);
            EXPECT_EQ(language.language_pack().SerializeAsString(), inVal_.SerializeAsString());
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.3 - PUT LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTAuthzInvalid) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::UNAUTHENTICATED);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(0);
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.4 - PUT LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTFailParse) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    method_ = Method_PUT;
    jsonBody_ = "Not a JSON string";
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(0);
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.5 - PUT LanguagePack add error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTNew) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::PERMISSION_DENIED);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).WillOnce(testing::Return(false));
    // Should not call if trying to add on PUT.
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.6 - PUT LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrReturn) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.7 - PUT LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowCat) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.8 - PUT LanguagePack throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowStd) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::INTERNAL);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 3.9 - PUT LanguagePack throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "tl", "Test Language", {{"hello", "world"}});
    method_ = Method_PUT;
    // Setting expectations.
    EXPECT_CALL(dm0_, hasLanguage(language_)).Times(1).WillOnce(testing::Return(method_ == Method_PUT));
    EXPECT_CALL(dm0_, addLanguage(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                              DELETE LanguagePack tests
 * ============================================================================
 * 
 * TEST 4.1 - DELETE LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETENormal) {
    initPayload(0, "tl");
    method_ = Method_DELETE;
    // Setting expectations.
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEAuthzValid) {
    initPayload(0, "tl");
    method_ = Method_DELETE;
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
    // Setting expectations.
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.3 - DELETE LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEAuthzInvalid) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::UNAUTHENTICATED);
    initPayload(0, "tl");
    method_ = Method_DELETE;
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations.
    EXPECT_CALL(dm0_, removeLanguage(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.4 - DELETE LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrReturn) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    method_ = Method_DELETE;
    // Setting expectations
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::common::Authorizer &authz) {
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));

    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.5 - DELETE LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowCat) {
    expRc_ = catena::exception_with_status("Test error", catena::StatusCode::INVALID_ARGUMENT);
    initPayload(0, "tl");
    method_ = Method_DELETE;
    // Setting expectations
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const std::string &languageId, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.6 - DELETE LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowStd) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::INTERNAL);
    initPayload(0, "tl");
    method_ = Method_DELETE;
    // Setting expectations
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4.6 - DELETE LanguagePack dm.getLanguagePack() throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initPayload(0, "tl");
    method_ = Method_DELETE;
    // Setting expectations
    EXPECT_CALL(dm0_, removeLanguage(language_, testing::_)).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}
