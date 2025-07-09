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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Unit tests for Device.cpp
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-01-27
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <Device.h>
#include <Authorization.h>
#include <Status.h>
#include <LanguagePack.h>
#include <interface/device.pb.h>
#include <mocks/MockLanguagePack.h>
#include <mocks/MockParam.h>
#include <mocks/MockParamDescriptor.h>
#include <CommonTestHelpers.h>

using namespace catena::common;

class DeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a device with basic parameters
        device_ = std::make_unique<Device>(
            0,  // slot
            catena::Device_DetailLevel_FULL,  // detail_level
            std::vector<std::string>{"admin"},  // access_scopes
            "admin",  // default_scope
            true,  // multi_set_enabled
            true   // subscriptions
        );
        
        // Create English language pack (shipped)
        englishPack_ = std::make_shared<LanguagePack>(
            "en",  // language code
            "English",  // name
            LanguagePack::ListInitializer{
                {"greeting", "Hello"},
                {"parting", "Goodbye"},
                {"welcome", "Welcome"}
            },
            *device_
        );
        
        // Create French language pack (shipped)
        frenchPack_ = std::make_shared<LanguagePack>(
            "fr",  // language code
            "French",  // name
            LanguagePack::ListInitializer{
                {"greeting", "Bonjour"},
                {"parting", "Au revoir"},
                {"welcome", "Bienvenue"}
            },
            *device_
        );

        // Using the admin/monitor tokens from Authorization_test.cpp
        std::string adminToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6YWRtOnciLCJpYXQiOjE1MTYyMzkwMjJ9.WrWmmNhw3EZ6AzZAytgZbvb_9NFL3_YtSSsZibW1P0w";
        adminAuthz_ = std::make_unique<Authorizer>(adminToken);
        std::string monitorToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";
        monitorAuthz_ = std::make_unique<Authorizer>(monitorToken);
    }

    std::unique_ptr<Device> device_;
    std::unique_ptr<Authorizer> adminAuthz_;
    std::unique_ptr<Authorizer> monitorAuthz_;
    std::shared_ptr<LanguagePack> englishPack_;
    std::shared_ptr<LanguagePack> frenchPack_;
};

// ======== 0. Initial Setup ========

// 0.1 - Test device creation
TEST_F(DeviceTest, Device_Create) {
    EXPECT_EQ(device_->slot(), 0);
    EXPECT_EQ(device_->detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(device_->subscriptions());
    EXPECT_EQ(device_->getDefaultScope(), "admin");
}

// ======== 1. Multi-Set Tests ========

// ======== 2. Get/Set Value Tests ========

// ======== 3. Language Tests ========

// --- Get Language Tests ---

// 3.1: Success Case - Test Language Pack Get
TEST_F(DeviceTest, LanguagePack_Get) {
    // Test getting a shipped language pack (English)
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("en", pack);
    EXPECT_EQ(result.status, catena::StatusCode::OK);
    EXPECT_EQ(pack.language(), "en");
    EXPECT_EQ(pack.language_pack().name(), "English");
    
    // Test getting another shipped language pack (French)
    Device::ComponentLanguagePack pack2;
    result = device_->getLanguagePack("fr", pack2);
    EXPECT_EQ(result.status, catena::StatusCode::OK);
    EXPECT_EQ(pack2.language(), "fr");
    EXPECT_EQ(pack2.language_pack().name(), "French");
}

// 3.2: Error Case - Test Language Pack Not Found
TEST_F(DeviceTest, LanguagePack_NotFound) {
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("nonexistent", pack);
    EXPECT_EQ(result.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(result.what()), "Language pack 'nonexistent' not found");
}

// 3.3: Error Case - Test Language Pack with Empty ID
TEST_F(DeviceTest, LanguagePack_EmptyLanguageId) {
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("", pack);
    EXPECT_EQ(result.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(result.what()), "Language ID is empty");
}

// 3.4: Error Case - Test Language Pack Get Internal Error
TEST_F(DeviceTest, LanguagePack_GetInternalError) {
    // Create a mock language pack that throws std::exception
    auto mockLanguagePack = std::make_shared<MockLanguagePack>();
    EXPECT_CALL(*mockLanguagePack, toProto(testing::_))
        .WillOnce(testing::Invoke([](catena::LanguagePack&) {
            throw std::runtime_error("Internal error in toProto");
        }));
    
    // Replace the existing English language pack with our mock
    device_->addItem("en", mockLanguagePack.get());
    
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("en", pack);
    EXPECT_EQ(result.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(result.what()), "Internal error in toProto");
}

// 3.5: Error Case - Test Language Pack Get Unknown Error
TEST_F(DeviceTest, LanguagePack_GetUnknownError) {
    // Create a mock language pack that throws unknown exception
    auto mockLanguagePack = std::make_shared<MockLanguagePack>();
    EXPECT_CALL(*mockLanguagePack, toProto(testing::_))
        .WillOnce(testing::Invoke([](catena::LanguagePack&) {
            throw 42; // Throw an int (unknown exception type)
        }));
    
    // Replace the existing French language pack with our mock
    device_->addItem("fr", mockLanguagePack.get());
    
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("fr", pack);
    EXPECT_EQ(result.status, catena::StatusCode::UNKNOWN);
    EXPECT_EQ(std::string(result.what()), "Unknown error");
}

// --- Add Language Tests ---

// 3.6: Success Case - Test Language Pack Add
TEST_F(DeviceTest, LanguagePack_Add) {
    // Create a language pack payload for a new language
    catena::AddLanguagePayload payload;
    payload.set_id("es");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");
    
    // Add language pack - should succeed with admin write permissions
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::OK);
}

// 3.7: Error Case - Test Language Pack Add Not Authorized
TEST_F(DeviceTest, LanguagePack_AddNotAuthorized) {
    // Try to add a language pack with monitor permissions (should fail)
    catena::AddLanguagePayload payload;
    payload.set_id("es");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");
    
    auto result = device_->addLanguage(payload, *monitorAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Not authorized to add language");
}

// 3.8: Error Case - Test Language Pack Add Invalid (Empty Name)
TEST_F(DeviceTest, LanguagePack_AddInvalidEmptyName) {
    // Create a language pack payload with empty name
    catena::AddLanguagePayload payload;
    payload.set_id("es");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name(""); // Empty name should cause INVALID_ARGUMENT
    
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(result.what()), "Invalid language pack");
}

// 3.9: Error Case - Test Language Pack Add Invalid (Empty ID)
TEST_F(DeviceTest, LanguagePack_AddInvalidEmptyId) {
    // Create a language pack payload with empty ID
    catena::AddLanguagePayload payload;
    payload.set_id(""); // Empty ID should cause INVALID_ARGUMENT
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");
    
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(result.what()), "Invalid language pack");
}

// 3.10: Error Case - Test Language Pack Add Cannot Overwrite Shipped Language
TEST_F(DeviceTest, LanguagePack_AddCannotOverwriteShippedLanguage) {
    // Try to add a language pack with the same ID as a shipped language pack
    catena::AddLanguagePayload payload;
    payload.set_id("en");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("English Override");
    
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Cannot overwrite language pack shipped with device");
}

// --- Remove Language Tests ---

// 3.11: Success Case - Test Language Pack Removal
TEST_F(DeviceTest, LanguagePack_Remove) {
    // First add a language pack that can be removed
    catena::AddLanguagePayload payload;
    payload.set_id("es");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");  

    // Add language pack - should succeed with admin write permissions
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::OK);

    // Remove language pack - should succeed with admin write permissions
    result = device_->removeLanguage("es", *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::OK);
}

// 3.12: Error Case - Test Language Pack Remove Not Authorized
TEST_F(DeviceTest, LanguagePack_RemoveNotAuthorized) {
    // Try to remove a language pack with monitor permissions (should fail)
    auto result = device_->removeLanguage("en", *monitorAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Not authorized to delete language");
}

// 3.13: Error Case - Test Language Pack Remove Cannot Delete Shipped Language
TEST_F(DeviceTest, LanguagePack_RemoveCannotDeleteShippedLanguage) {
    // Try to remove a shipped language pack (should fail)
    auto result = device_->removeLanguage("en", *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Cannot delete language pack shipped with device");
}

// 3.14: Error Case - Test Language Pack Remove Not Found
TEST_F(DeviceTest, LanguagePack_RemoveNotFound) {
    // Try to remove a language pack that doesn't exist
    auto result = device_->removeLanguage("nonexistent", *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(result.what()), "Language pack 'nonexistent' not found");
}

// ======== 4. Param/Command Tests ========
// Covers getParam, getTopLevelParams, and getCommand

// --- Get Param Tests (String-based overload) ---

// 4.1: Success Case - Test Get Param with Valid String Path
TEST_F(DeviceTest, GetParam_String_Success) {
    // Create a mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    
    device_->addItem("testParam", mockParam.get());
    
    // Test getting the parameter
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/testParam", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_NE(result, nullptr);
}

// 4.2: Error Case - Test Get Param with Empty String Path
TEST_F(DeviceTest, GetParam_String_EmptyPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer ");
    EXPECT_EQ(result, nullptr);
}

// 4.3: Error Case - Test Get Param with Invalid String Path
TEST_F(DeviceTest, GetParam_String_InvalidPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/invalid/path", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Param /invalid/path does not exist");
    EXPECT_EQ(result, nullptr);
}

// 4.4: Error Case - Test Get Param Not Authorized (String)
TEST_F(DeviceTest, GetParam_String_NotAuthorized) {
    // Create a mock parameter that requires specific authorization
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - parameter requires admin scope but monitor token only has st2138:mon
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/restrictedParam", mockDescriptor.get(), false, 0, adminScope);
    
    // copy() should not be called since authorization will fail
    EXPECT_CALL(*mockParam, copy())
        .Times(0);
    
    device_->addItem("restrictedParam", mockParam.get());
    
    // Test with monitor authorization (should fail)
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/restrictedParam", status, *monitorAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(status.what()), "Not authorized to read the param /restrictedParam");
    EXPECT_EQ(result, nullptr);
}

// 4.5: Error Case - Test Get Param with Invalid Json Pointer (String)
TEST_F(DeviceTest, GetParam_String_InvalidJsonPointer) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/invalid[", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(result, nullptr);
}

// 4.6: Error Case - Test Get Param Internal Error (String)
TEST_F(DeviceTest, GetParam_String_InternalError) {
    // Create a mock parameter that throws std::exception
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/errorParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            throw std::runtime_error("Internal error in copy");
        }));
    
    device_->addItem("errorParam", mockParam.get());
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/errorParam", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Internal error in copy");
    EXPECT_EQ(result, nullptr);
}

// 4.7: Error Case - Test Get Param Unknown Error (String)
TEST_F(DeviceTest, GetParam_String_UnknownError) {
    // Create a mock parameter that throws unknown exception
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/unknownErrorParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            throw 42; // Throw an int (unknown exception type)
        }));
    
    device_->addItem("unknownErrorParam", mockParam.get());
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/unknownErrorParam", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::UNKNOWN);
    EXPECT_EQ(std::string(status.what()), "Unknown error");
    EXPECT_EQ(result, nullptr);
}

// --- Get Param Tests (Path-based overload) ---

// 4.8: Success Case - Test Get Param with Valid Path Object
TEST_F(DeviceTest, GetParam_Path_Success) {
    // Create a mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    
    device_->addItem("testParam", mockParam.get());
    
    // Test getting the parameter using Path overload
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/testParam");
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_NE(result, nullptr);
}

// 4.9: Error Case - Test Get Param with Empty Path Object
TEST_F(DeviceTest, GetParam_Path_EmptyPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("");
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer ");
    EXPECT_EQ(result, nullptr);
}

// 4.10: Error Case - Test Get Param with Invalid Path Object
TEST_F(DeviceTest, GetParam_Path_InvalidPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/invalid/path");
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Param /invalid/path does not exist");
    EXPECT_EQ(result, nullptr);
}

// 4.11: Error Case - Test Get Param Not Authorized (Path)
TEST_F(DeviceTest, GetParam_Path_NotAuthorized) {
    // Create a mock parameter that requires specific authorization
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - parameter requires admin scope but monitor token only has st2138:mon
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/restrictedParam", mockDescriptor.get(), false, 0, adminScope);
    
    // copy() should not be called since authorization will fail
    EXPECT_CALL(*mockParam, copy())
        .Times(0);
    
    device_->addItem("restrictedParam", mockParam.get());
    
    // Test with monitor authorization (should fail)
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/restrictedParam");
    auto result = device_->getParam(path, status, *monitorAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(status.what()), "Not authorized to read the param /restrictedParam");
    EXPECT_EQ(result, nullptr);
}

// 4.12: Error Case - Test Get Param with Non-String Front Element (Path)
TEST_F(DeviceTest, GetParam_Path_NonStringFrontElement) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/123"); // Path with numeric front element
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer /123");
    EXPECT_EQ(result, nullptr);
}

// 4.13: Success Case - Test Get Param with Sub-path (Path)
TEST_F(DeviceTest, GetParam_Path_SubPath) {
    // Create a mock parameter that supports sub-parameters
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockSubParam = std::make_unique<MockParam>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/parentParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getParam(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(std::move(mockSubParam)));
    
    device_->addItem("parentParam", mockParam.get());
    
    // Test getting a sub-parameter using Path overload
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/parentParam/subParam");
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_NE(result, nullptr);
}

// ======== 5. Serialization Tests ========
// Covers toProto calls, getComponentSerializer, and getDeviceSerializer

// 5.1 - Test shallow toProto serialization
TEST_F(DeviceTest, Device_ToProtoShallow) {
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, true); // shallow copy
    
    EXPECT_EQ(proto.slot(), 0);
    EXPECT_EQ(proto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(proto.multi_set_enabled());
    EXPECT_TRUE(proto.subscriptions());
    EXPECT_EQ(proto.default_scope(), "admin");
}
