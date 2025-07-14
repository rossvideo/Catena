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
 * @date 2025-07-10
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
            1,  // slot
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
    EXPECT_EQ(device_->slot(), 1);
    EXPECT_EQ(device_->detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(device_->subscriptions());
    EXPECT_EQ(device_->getDefaultScope(), "admin");
}

// ======== 1. Multi-Set Tests ========

// --- TryMultiSetValue Tests ---

// 1.1: Success Case - Test Multi-Set Value with Single Value (Multi-Set Enabled)
TEST_F(DeviceTest, TryMultiSetValue_SingleValueSuccess) {
    // Create mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/param1", mockDescriptor.get(), false, 0, adminScope);
    
    // Set up basic expectations for copy() - return mocks that validate successfully
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockParam.get());
    
    // Create a payload with single value (should succeed even with multi-set enabled)
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    // Test the multi-set validation
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should succeed even with multi-set enabled because there's only one value
    EXPECT_TRUE(result);
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.2: Success Case - Test Multi-Set Value with Multiple Valid Parameters
TEST_F(DeviceTest, TryMultiSetValue_MultipleValuesSuccess) {
    // Create mock parameters and add them to the device
    auto mockParam1 = std::make_shared<MockParam>();
    auto mockParam2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam1.get(), "/param1", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockParam2.get(), "/param2", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up basic expectations for copy() - return mocks that validate successfully
    EXPECT_CALL(*mockParam1, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::Eq(3), testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    EXPECT_CALL(*mockParam2, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
    // Create a payload with multiple values
    catena::MultiSetValuePayload payload;
    
    // First value - with path ending in index
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1/3");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value - regular path
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param2");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    // Test the multi-set validation
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should succeed
    EXPECT_TRUE(result);
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.3: Error Case - Test Multi-Set Value with Multi-Set Disabled
TEST_F(DeviceTest, TryMultiSetValue_MultiSetDisabled) {
    // Create a device with multi-set disabled
    auto deviceDisabled = std::make_unique<Device>(
        1,  // slot
        catena::Device_DetailLevel_FULL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        false,  // multi_set_enabled - DISABLED
        true   // subscriptions
    );
    
    // Create a payload with multiple values
    catena::MultiSetValuePayload payload;
    
    // First value
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param2");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    // Test the multi-set validation - should fail because multi-set is disabled
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = deviceDisabled->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should fail
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(status.what()), "Multi-set is disabled for the device in slot 1");
}

// 1.4: Error Case - Test Multi-Set Value with Non-existent Parameter
TEST_F(DeviceTest, TryMultiSetValue_NonExistentParameter) {
    // Create a payload with a non-existent parameter
    catena::MultiSetValuePayload payload;
    
    // First value - non-existent parameter
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/nonexistentParam");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value - also non-existent
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/anotherNonexistentParam");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    // Test the multi-set validation
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should fail because the first parameter doesn't exist
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Param /nonexistentParam does not exist");
}

// 1.5: Error Case - Test Multi-Set Value with Validation Failure
TEST_F(DeviceTest, TryMultiSetValue_ValidationFailure) {
    // Create mock parameters and add them to the device
    auto mockParam1 = std::make_shared<MockParam>();
    auto mockParam2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam1.get(), "/param1", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockParam2.get(), "/param2", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up expectations for getParam calls - first should fail validation, second should not be called for validation
    EXPECT_CALL(*mockParam1, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("Validation failed", catena::StatusCode::INVALID_ARGUMENT);
                    return false;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    // param2 should be called for reset even though validation failed on param1
    EXPECT_CALL(*mockParam2, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
    // Create a payload with multiple values
    catena::MultiSetValuePayload payload;
    
    // First value
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param2");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    // Test the multi-set validation
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should fail due to validation error
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Validation failed");
}

// 1.6: Error Case - Test Multi-Set Value with Catena Exception
TEST_F(DeviceTest, TryMultiSetValue_CatenaException) {
    // Create mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/param1", mockDescriptor.get(), false, 0, adminScope);
    
    // Set up expectations for copy() - throw a catena exception during first copy, second copy for reset
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            throw catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
        }))
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockParam.get());
    
    // Create a payload with single value
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    // Test the multi-set validation
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    // Should fail due to catena exception
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Test catena exception");
}

// --- commitMultiSetValue Tests ---

// 1.7: Success Case - Test commitMultiSetValue with single value
TEST_F(DeviceTest, CommitMultiSetValue_SingleValueSuccess) {
    // Create mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/param1", mockDescriptor.get(), false, 0, adminScope);
    
    // Set up expectations for copy() - return mocks that handle fromProto and resetValidate
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, fromProto(testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockParam.get());
    
    // Create a payload with single value
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    // Test the commit
    catena::exception_with_status status{"", catena::StatusCode::OK};
    status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
    // Should succeed
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.8: Success Case - Test commitMultiSetValue with regular parameters
TEST_F(DeviceTest, CommitMultiSetValue_RegularParametersSuccess) {
    // Create parameter hierarchy using the helper
    auto regularParam1 = ParamHierarchyBuilder::createDescriptor("/param1");
    auto regularParam2 = ParamHierarchyBuilder::createDescriptor("/param2");
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Create mock parameters with proper descriptors
    auto mockRegularParam1 = std::make_shared<MockParam>();
    auto mockRegularParam2 = std::make_shared<MockParam>();
    
    setupMockParam(mockRegularParam1.get(), "/param1", regularParam1.descriptor.get(), false, 0, adminScope);
    setupMockParam(mockRegularParam2.get(), "/param2", regularParam2.descriptor.get(), false, 0, adminScope);
    
    // Set up expectations for regular parameters (direct path access)
    EXPECT_CALL(*mockRegularParam1, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, fromProto(testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    EXPECT_CALL(*mockRegularParam2, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            EXPECT_CALL(*mock, fromProto(testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("param1", mockRegularParam1.get());
    device_->addItem("param2", mockRegularParam2.get());
    
    // Track signal emissions
    std::vector<std::pair<std::string, const IParam*>> signalEmissions;
    auto signalConnection = device_->valueSetByClient.connect(
        [&signalEmissions](const std::string& oid, const IParam* param) {
            signalEmissions.emplace_back(oid, param);
        }
    );
    
    // Create a payload with regular parameters
    catena::MultiSetValuePayload payload;
    
    // First value - regular path with int value
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value - regular path with string value
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param2");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    // Test the commit
    catena::exception_with_status status{"", catena::StatusCode::OK};
    status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
    // Should succeed
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    
    // Should have emitted two signals (one for each parameter)
    EXPECT_EQ(signalEmissions.size(), 2);
    
    // Check signal emissions in order
    EXPECT_EQ(signalEmissions[0].first, "/param1");
    EXPECT_NE(signalEmissions[0].second, nullptr);
    
    EXPECT_EQ(signalEmissions[1].first, "/param2");
    EXPECT_NE(signalEmissions[1].second, nullptr);
}

// Neither of these tests work correctly.

// // 1.9: Success Case - Test commitMultiSetValue with array indexed access
// TEST_F(DeviceTest, CommitMultiSetValue_ArrayIndexedAccessSuccess) {
//     // Create parameter hierarchy using the helper
//     auto parentParam = ParamHierarchyBuilder::createDescriptor("/parentParam");
//     auto arrayElementDescriptor = ParamHierarchyBuilder::createDescriptor("/parentParam/3/subParam");
    
//     // Set up authorization - admin token has st2138:adm:w scope
//     static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
//     // Create mock parameters with proper descriptors
//     auto mockParentParam = std::make_unique<MockParam>();
//     auto mockArrayElement = std::make_unique<MockParam>();
    
//     setupMockParam(mockParentParam.get(), "/parentParam", parentParam.descriptor.get(), true, 5, adminScope); // Array with 5 elements
//     setupMockParam(mockArrayElement.get(), "/parentParam/3", parentParam.descriptor.get(), false, 0, adminScope);
    
//     // Create the chain of mock parameters
//     auto mockParentCopy = std::make_unique<MockParam>();
//     auto mockLeafParam = std::make_unique<MockParam>();
    
//     // Set up expectations for parent parameter copy
//     EXPECT_CALL(*mockParentParam, copy())
//         .WillOnce(testing::Invoke([&mockParentCopy]() {
//             return std::move(mockParentCopy);
//         }));
    
//     // Set up expectations for parent copy to handle array element access
//     EXPECT_CALL(*mockParentCopy, getParam(testing::_, testing::_, testing::_))
//         .WillOnce(testing::Invoke([&mockArrayElement](catena::common::Path& path, catena::common::Authorizer&, catena::exception_with_status& status) {
//             status = catena::exception_with_status("", catena::StatusCode::OK);
//             return std::unique_ptr<IParam>(mockArrayElement.get());
//         }));
    
//     // Set up expectations for array element (the actual target)
//     EXPECT_CALL(*mockArrayElement, fromProto(testing::_, testing::_))
//         .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
//             return catena::exception_with_status("", catena::StatusCode::OK);
//         }));
    
//     // Set up expectations for resetValidate on the parent copy
//     EXPECT_CALL(*mockParentCopy, resetValidate())
//         .Times(1);
    
//     device_->addItem("parentParam", mockParentParam.get());
    
//     // Track signal emissions
//     std::vector<std::pair<std::string, const IParam*>> signalEmissions;
//     auto signalConnection = device_->valueSetByClient.connect(
//         [&signalEmissions](const std::string& oid, const IParam* param) {
//             signalEmissions.emplace_back(oid, param);
//         }
//     );
    
//     // Create a payload with array indexed access
//     catena::MultiSetValuePayload payload;
    
//     // Value - sub-parameter of indexed array element
//     auto* setValue = payload.add_values();
//     setValue->set_oid("/parentParam/3/subParam");
//     auto* value = setValue->mutable_value();
//     value->set_int32_value(100);
    
//     // Test the commit
//     catena::exception_with_status status{"", catena::StatusCode::OK};
//     status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
//     // Should succeed
//     EXPECT_EQ(status.status, catena::StatusCode::OK);
    
//     // Should have emitted one signal for the array indexed access
//     EXPECT_EQ(signalEmissions.size(), 1);
    
//     // Check signal emission
//     EXPECT_EQ(signalEmissions[0].first, "/parentParam/3/subParam");
//     EXPECT_NE(signalEmissions[0].second, nullptr);
// }

// // 1.10: Success Case - Test commitMultiSetValue with array append operation
// TEST_F(DeviceTest, CommitMultiSetValue_ArrayAppendSuccess) {
//     // Create parameter hierarchy using the helper
//     auto parentParam = ParamHierarchyBuilder::createDescriptor("/parentParam");
    
//     // Set up authorization - admin token has st2138:adm:w scope
//     static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
//     // Create mock parameters with proper descriptors
//     auto mockParentParam = std::make_unique<MockParam>();
//     auto mockAppendedElement = std::make_unique<MockParam>();
    
//     setupMockParam(mockParentParam.get(), "/parentParam", parentParam.descriptor.get(), true, 5, adminScope); // Array with 5 elements
//     setupMockParam(mockAppendedElement.get(), "/parentParam/5", parentParam.descriptor.get(), false, 0, adminScope);
    
//     // Set up expectations for parent parameter to handle append operations
//     EXPECT_CALL(*mockParentParam, copy())
//         .WillOnce(testing::Invoke([&mockAppendedElement]() { 
//             auto mock = std::make_unique<MockParam>();
//             // For "/parentParam/-" - addBack should append a new element
//             EXPECT_CALL(*mock, addBack(testing::_, testing::_))
//                 .WillOnce(testing::Invoke([&mockAppendedElement](catena::common::Authorizer& authz, catena::exception_with_status& status) {
//                     status = catena::exception_with_status("", catena::StatusCode::OK);
//                     return std::move(mockAppendedElement);
//                 }));
//             EXPECT_CALL(*mock, resetValidate())
//                 .Times(1);
//             return mock;
//         }));
    
//     // Set up expectations for appended element
//     EXPECT_CALL(*mockAppendedElement, fromProto(testing::_, testing::_))
//         .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
//             return catena::exception_with_status("", catena::StatusCode::OK);
//         }));
    
//     device_->addItem("parentParam", mockParentParam.get());
    
//     // Track signal emissions
//     std::vector<std::pair<std::string, const IParam*>> signalEmissions;
//     auto signalConnection = device_->valueSetByClient.connect(
//         [&signalEmissions](const std::string& oid, const IParam* param) {
//             signalEmissions.emplace_back(oid, param);
//         }
//     );
    
//     // Create a payload with array append operation
//     catena::MultiSetValuePayload payload;
    
//     // Value - kEnd case (append new element to array)
//     auto* setValue = payload.add_values();
//     setValue->set_oid("/parentParam/-");
//     auto* value = setValue->mutable_value();
//     value->set_string_value("appended");
    
//     // Test the commit
//     catena::exception_with_status status{"", catena::StatusCode::OK};
//     status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
//     // Should succeed
//     EXPECT_EQ(status.status, catena::StatusCode::OK);
    
//     // Should have emitted one signal for the array append operation
//     EXPECT_EQ(signalEmissions.size(), 1);
    
//     // Check signal emission
//     EXPECT_EQ(signalEmissions[0].first, "/parentParam/-");
//     EXPECT_NE(signalEmissions[0].second, nullptr);
// }

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

// --- Get Top Level Params Tests ---

// 4.14: Success Case - Test Get Top Level Params
TEST_F(DeviceTest, GetTopLevelParams_Success) {
    // Create mock parameters and add them to the device
    auto mockParam1 = std::make_shared<MockParam>();
    auto mockParam2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam1.get(), "/param1", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockParam2.get(), "/param2", mockDescriptor2.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam1, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    EXPECT_CALL(*mockParam2, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
    // Test getting top level parameters
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getTopLevelParams(status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.size(), 2);
}

// 4.15: Success Case - Test Get Top Level Params with Authorization Filtering
TEST_F(DeviceTest, GetTopLevelParams_AuthorizationFiltering) {
    // Create mock parameters with different authorization requirements
    auto mockParam1 = std::make_shared<MockParam>();
    auto mockParam2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - param1 allows monitor access, param2 requires admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam1.get(), "/authorizedParam", mockDescriptor1.get(), false, 0, monitorScope);
    setupMockParam(mockParam2.get(), "/restrictedParam", mockDescriptor2.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam1, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    // mockParam2 will not be authorized, so it won't be copied
    
    device_->addItem("authorizedParam", mockParam1.get());
    device_->addItem("restrictedParam", mockParam2.get());
    
    // Test with monitor authorization (should only get authorized params)
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getTopLevelParams(status, *monitorAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.size(), 1); // Only the authorized param should be returned
}

// 4.16: Error Case - Test Get Top Level Params with Exception
TEST_F(DeviceTest, GetTopLevelParams_Exception) {
    // Create a mock parameter that throws an exception during processing
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/exceptionParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            throw catena::exception_with_status("Test exception", catena::StatusCode::INTERNAL);
        }));
    
    device_->addItem("exceptionParam", mockParam.get());
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getTopLevelParams(status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Test exception");
    EXPECT_TRUE(result.empty());
}

// --- Get Command Tests ---

// 4.17: Success Case - Test Get Command with Valid Path
TEST_F(DeviceTest, GetCommand_Success) {
    // Create a mock command and add it to the device
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockCommand.get(), "/testCommand", mockDescriptor.get(), false, 0, adminScope);
    
    // Setup expectations for command
    EXPECT_CALL(*mockDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockCommand, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    
    device_->addItem("testCommand", mockCommand.get());
    
    // Test getting the command
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/testCommand", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_NE(result, nullptr);
}

// 4.18: Error Case - Test Get Command with Empty Path
TEST_F(DeviceTest, GetCommand_EmptyPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer");
    EXPECT_EQ(result, nullptr);
}

// 4.19: Error Case - Test Get Command Not Found
TEST_F(DeviceTest, GetCommand_NotFound) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/nonexistentCommand", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Command not found: /nonexistentCommand");
    EXPECT_EQ(result, nullptr);
}

// 4.20: Error Case - Test Get Command with Sub-commands (Unimplemented)
TEST_F(DeviceTest, GetCommand_SubCommandsUnimplemented) {
    // Create a mock command and add it to the device
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockCommand.get(), "/testCommand", mockDescriptor.get(), false, 0, adminScope);
    
    // Setup expectations for command
    EXPECT_CALL(*mockDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    // copy() should not be called since this is not implemented
    EXPECT_CALL(*mockCommand, copy())
        .Times(0);
    
    device_->addItem("testCommand", mockCommand.get());
    
    // Test getting a sub-command (should fail)
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/testCommand/subcommand", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::UNIMPLEMENTED);
    EXPECT_EQ(std::string(status.what()), "sub-commands not implemented");
    EXPECT_EQ(result, nullptr);
}

// 4.21: Error Case - Test Get Command with Invalid Json Pointer
TEST_F(DeviceTest, GetCommand_InvalidJsonPointer) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/invalid[", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(result, nullptr);
}

// 4.22: Error Case - Test Get Command with Non-String Front Element
TEST_F(DeviceTest, GetCommand_NonStringFrontElement) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/123", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer");
    EXPECT_EQ(result, nullptr);
}

// 4.23: Error Case - Test Get Command with Exception
TEST_F(DeviceTest, GetCommand_Exception) {
    // Create a mock command that throws an exception
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockCommand.get(), "/exceptionCommand", mockDescriptor.get(), false, 0, adminScope);
    
    // Setup expectations for command
    EXPECT_CALL(*mockDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockCommand, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<IParam> {
            throw catena::exception_with_status("Command exception", catena::StatusCode::INTERNAL);
        }));
    
    device_->addItem("exceptionCommand", mockCommand.get());
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/exceptionCommand", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Command exception");
    EXPECT_EQ(result, nullptr);
}

// ======== 5. Serialization Tests ========
// Covers toProto calls, getComponentSerializer, and getDeviceSerializer

// 5.1 - Test shallow toProto serialization
TEST_F(DeviceTest, Device_ToProtoShallow) {
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, true); // shallow copy
    
    EXPECT_EQ(proto.slot(), 1);
    EXPECT_EQ(proto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(proto.multi_set_enabled());
    EXPECT_TRUE(proto.subscriptions());
    EXPECT_EQ(proto.default_scope(), "admin");
}
