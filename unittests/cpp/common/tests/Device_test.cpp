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
#include <mocks/MockConstraint.h>
#include <mocks/MockMenuGroup.h>
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

// ======== 5. toProto Tests ========

// --- Base toProto Device Tests ---

// 5.1 - Test shallow vs deep toProto serialization
TEST_F(DeviceTest, Device_ToProtoShallowVsDeep) {
    // Test shallow copy - should only serialize basic properties
    catena::Device shallowProto;
    device_->toProto(shallowProto, *adminAuthz_, true); // shallow copy
    
    EXPECT_EQ(shallowProto.slot(), 1);
    EXPECT_EQ(shallowProto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(shallowProto.multi_set_enabled());
    EXPECT_TRUE(shallowProto.subscriptions());
    EXPECT_EQ(shallowProto.default_scope(), "admin");
    
    // Verify shallow copy does NOT serialize collections
    EXPECT_EQ(shallowProto.params_size(), 0);
    EXPECT_EQ(shallowProto.commands_size(), 0);
    EXPECT_EQ(shallowProto.constraints_size(), 0);
    EXPECT_EQ(shallowProto.menu_groups_size(), 0);
    EXPECT_EQ(shallowProto.language_packs().packs_size(), 0);
    
    // Test deep copy - should serialize everything
    catena::Device deepProto;
    device_->toProto(deepProto, *adminAuthz_, false); // deep copy
    
    EXPECT_EQ(deepProto.slot(), 1);
    EXPECT_EQ(deepProto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(deepProto.multi_set_enabled());
    EXPECT_TRUE(deepProto.subscriptions());
    EXPECT_EQ(deepProto.default_scope(), "admin");
    
    // Verify deep copy DOES serialize collections (language packs from SetUp)
    EXPECT_EQ(deepProto.language_packs().packs_size(), 2);
    EXPECT_TRUE(deepProto.language_packs().packs().contains("en"));
    EXPECT_TRUE(deepProto.language_packs().packs().contains("fr"));
}

// 5.2 - Test toProto with parameters serialization
TEST_F(DeviceTest, Device_ToProtoWithParameters) {
    // Create mock parameters and add them to the device
    auto mockParam1 = std::make_shared<MockParam>();
    auto mockParam2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam1.get(), "/param1", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockParam2.get(), "/param2", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up expectations for shouldSendParam and toProto calls
    EXPECT_CALL(*mockParam1, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
    EXPECT_CALL(*mockParam2, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
    
    EXPECT_CALL(*mockParam1, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockParam2, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify parameters were serialized
    EXPECT_EQ(proto.params_size(), 2);
    EXPECT_TRUE(proto.params().contains("param1"));
    EXPECT_TRUE(proto.params().contains("param2"));
    EXPECT_EQ(proto.params().at("param1").type(), catena::ParamType::INT32);
    EXPECT_EQ(proto.params().at("param2").type(), catena::ParamType::STRING);
}

// 5.3 - Test toProto with commands serialization
TEST_F(DeviceTest, Device_ToProtoWithCommands) {
    // Create mock commands and add them to the device
    auto mockCommand1 = std::make_shared<MockParam>();
    auto mockCommand2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockCommand1.get(), "/command1", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockCommand2.get(), "/command2", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up expectations for shouldSendParam and toProto calls
    EXPECT_CALL(*mockCommand1, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
    EXPECT_CALL(*mockCommand2, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
    
    // Override isCommand to return true for commands
    EXPECT_CALL(*mockDescriptor1, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockDescriptor2, isCommand())
        .WillRepeatedly(testing::Return(true));
    
    EXPECT_CALL(*mockCommand1, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockCommand2, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    device_->addItem("command1", mockCommand1.get());
    device_->addItem("command2", mockCommand2.get());
    
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify commands were serialized
    EXPECT_EQ(proto.commands_size(), 2);
    EXPECT_TRUE(proto.commands().contains("command1"));
    EXPECT_TRUE(proto.commands().contains("command2"));
    EXPECT_EQ(proto.commands().at("command1").type(), catena::ParamType::INT32);
    EXPECT_EQ(proto.commands().at("command2").type(), catena::ParamType::STRING);
}

// 5.4 - Test toProto with constraints serialization
TEST_F(DeviceTest, Device_ToProtoWithConstraints) {
    // Create mock constraints and add them to the device
    auto mockConstraint1 = std::make_shared<MockConstraint>();
    auto mockConstraint2 = std::make_shared<MockConstraint>();
    
    EXPECT_CALL(*mockConstraint1, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            constraint.set_ref_oid("constraint1");
        }));
    EXPECT_CALL(*mockConstraint2, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            constraint.set_ref_oid("constraint2");
        }));
    
    device_->addItem("constraint1", mockConstraint1.get());
    device_->addItem("constraint2", mockConstraint2.get());
    
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify constraints were serialized
    EXPECT_EQ(proto.constraints_size(), 2);
    EXPECT_TRUE(proto.constraints().contains("constraint1"));
    EXPECT_TRUE(proto.constraints().contains("constraint2"));
    EXPECT_EQ(proto.constraints().at("constraint1").ref_oid(), "constraint1");
    EXPECT_EQ(proto.constraints().at("constraint2").ref_oid(), "constraint2");
}

// 5.5 - Test toProto with language packs serialization
TEST_F(DeviceTest, Device_ToProtoWithLanguagePacks) {
    // Language packs are already set up in SetUp()
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify language packs were serialized
    EXPECT_EQ(proto.language_packs().packs_size(), 2);
    EXPECT_TRUE(proto.language_packs().packs().contains("en"));
    EXPECT_TRUE(proto.language_packs().packs().contains("fr"));
    EXPECT_EQ(proto.language_packs().packs().at("en").name(), "English");
    EXPECT_EQ(proto.language_packs().packs().at("fr").name(), "French");
}

// 5.6 - Test toProto with menu groups serialization
TEST_F(DeviceTest, Device_ToProtoWithMenuGroups) {
    // Create mock menu groups and add them to the device
    auto mockMenuGroup1 = std::make_shared<MockMenuGroup>();
    auto mockMenuGroup2 = std::make_shared<MockMenuGroup>();
    
    EXPECT_CALL(*mockMenuGroup1, toProto(testing::An<catena::MenuGroup&>(), false))
        .WillOnce(testing::Invoke([](catena::MenuGroup& menuGroup, bool shallow) {
            auto* name = menuGroup.mutable_name();
            name->mutable_display_strings()->insert({"en", "Menu Group 1"});
        }));
    EXPECT_CALL(*mockMenuGroup2, toProto(testing::An<catena::MenuGroup&>(), false))
        .WillOnce(testing::Invoke([](catena::MenuGroup& menuGroup, bool shallow) {
            auto* name = menuGroup.mutable_name();
            name->mutable_display_strings()->insert({"en", "Menu Group 2"});
        }));
    
    device_->addItem("menuGroup1", mockMenuGroup1.get());
    device_->addItem("menuGroup2", mockMenuGroup2.get());
    
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify menu groups were serialized
    EXPECT_EQ(proto.menu_groups_size(), 2);
    EXPECT_TRUE(proto.menu_groups().contains("menuGroup1"));
    EXPECT_TRUE(proto.menu_groups().contains("menuGroup2"));
    EXPECT_EQ(proto.menu_groups().at("menuGroup1").name().display_strings().at("en"), "Menu Group 1");
    EXPECT_EQ(proto.menu_groups().at("menuGroup2").name().display_strings().at("en"), "Menu Group 2");
}

// 5.7 - Test toProto with minimal detail level (should skip constraints, language packs, menu groups)
TEST_F(DeviceTest, Device_ToProtoMinimalDetailLevel) {
    // Create a device with minimal detail level
    auto minimalDevice = std::make_unique<Device>(
        2,  // slot
        catena::Device_DetailLevel_MINIMAL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Add some constraints, language packs, and menu groups
    auto mockConstraint = std::make_shared<MockConstraint>();
    auto mockMenuGroup = std::make_shared<MockMenuGroup>();
    
    // These should NOT be called because of minimal detail level
    EXPECT_CALL(*mockConstraint, toProto(testing::_))
        .Times(0);
    EXPECT_CALL(*mockMenuGroup, toProto(testing::_, false))
        .Times(0);
    
    minimalDevice->addItem("constraint1", mockConstraint.get());
    minimalDevice->addItem("menuGroup1", mockMenuGroup.get());
    
    catena::Device proto;
    minimalDevice->toProto(proto, *adminAuthz_, false);
    
    // Verify basic properties are still serialized
    EXPECT_EQ(proto.slot(), 2);
    EXPECT_EQ(proto.detail_level(), catena::Device_DetailLevel_MINIMAL);
    EXPECT_TRUE(proto.multi_set_enabled());
    EXPECT_TRUE(proto.subscriptions());
    EXPECT_EQ(proto.default_scope(), "admin");
    
    // Verify constraints and menu groups were NOT serialized
    EXPECT_EQ(proto.constraints_size(), 0);
    EXPECT_EQ(proto.menu_groups_size(), 0);
}

// 5.8 - Test toProto with authorization filtering (unauthorized parameters should not be serialized)
TEST_F(DeviceTest, Device_ToProtoWithAuthorizationFiltering) {
    // Create mock parameters with different authorization requirements
    auto mockAuthorizedParam = std::make_shared<MockParam>();
    auto mockUnauthorizedParam = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - authorized param allows monitor access, unauthorized param requires admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockAuthorizedParam.get(), "/authorizedParam", mockDescriptor1.get(), false, 0, monitorScope);
    setupMockParam(mockUnauthorizedParam.get(), "/unauthorizedParam", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up expectations for shouldSendParam and toProto calls
    EXPECT_CALL(*mockAuthorizedParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
    EXPECT_CALL(*mockUnauthorizedParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
    
    // Only the authorized param should be serialized
    EXPECT_CALL(*mockAuthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockUnauthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called
    
    device_->addItem("authorizedParam", mockAuthorizedParam.get());
    device_->addItem("unauthorizedParam", mockUnauthorizedParam.get());
    
    catena::Device proto;
    device_->toProto(proto, *monitorAuthz_, false);
    
    // Verify only authorized parameters were serialized
    EXPECT_EQ(proto.params_size(), 1);
    EXPECT_TRUE(proto.params().contains("authorizedParam"));
    EXPECT_FALSE(proto.params().contains("unauthorizedParam"));
    EXPECT_EQ(proto.params().at("authorizedParam").type(), catena::ParamType::INT32);
}

// 5.9 - Test toProto with mixed content (parameters, commands, constraints, language packs, menu groups)
TEST_F(DeviceTest, Device_ToProtoWithMixedContent) {
    // Create various mock objects
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockConstraint = std::make_shared<MockConstraint>();
    auto mockMenuGroup = std::make_shared<MockMenuGroup>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor1.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockDescriptor2.get(), false, 0, adminScope);
    
    // Set up expectations
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
    EXPECT_CALL(*mockCommand, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
    
    // Override isCommand to return true for the command
    EXPECT_CALL(*mockDescriptor2, isCommand())
        .WillRepeatedly(testing::Return(true));
    
    EXPECT_CALL(*mockParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockCommand, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockConstraint, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            constraint.set_ref_oid("testConstraint");
        }));
    EXPECT_CALL(*mockMenuGroup, toProto(testing::An<catena::MenuGroup&>(), false))
        .WillOnce(testing::Invoke([](catena::MenuGroup& menuGroup, bool shallow) {
            auto* name = menuGroup.mutable_name();
            name->mutable_display_strings()->insert({"en", "Test Menu Group"});
        }));
    
    // Add all items to device
    device_->addItem("testParam", mockParam.get());
    device_->addItem("testCommand", mockCommand.get());
    device_->addItem("testConstraint", mockConstraint.get());
    device_->addItem("testMenuGroup", mockMenuGroup.get());
    
    catena::Device proto;
    device_->toProto(proto, *adminAuthz_, false);
    
    // Verify all components were serialized
    EXPECT_EQ(proto.slot(), 1);
    EXPECT_EQ(proto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(proto.multi_set_enabled());
    EXPECT_TRUE(proto.subscriptions());
    EXPECT_EQ(proto.default_scope(), "admin");
    
    // Verify parameters
    EXPECT_EQ(proto.params_size(), 1);
    EXPECT_TRUE(proto.params().contains("testParam"));
    EXPECT_EQ(proto.params().at("testParam").type(), catena::ParamType::INT32);
    
    // Verify commands
    EXPECT_EQ(proto.commands_size(), 1);
    EXPECT_TRUE(proto.commands().contains("testCommand"));
    EXPECT_EQ(proto.commands().at("testCommand").type(), catena::ParamType::STRING);
    
    // Verify constraints
    EXPECT_EQ(proto.constraints_size(), 1);
    EXPECT_TRUE(proto.constraints().contains("testConstraint"));
    EXPECT_EQ(proto.constraints().at("testConstraint").ref_oid(), "testConstraint");
    
    // Verify menu groups
    EXPECT_EQ(proto.menu_groups_size(), 1);
    EXPECT_TRUE(proto.menu_groups().contains("testMenuGroup"));
    EXPECT_EQ(proto.menu_groups().at("testMenuGroup").name().display_strings().at("en"), "Test Menu Group");
    
    // Verify language packs (from SetUp)
    EXPECT_EQ(proto.language_packs().packs_size(), 2);
    EXPECT_TRUE(proto.language_packs().packs().contains("en"));
    EXPECT_TRUE(proto.language_packs().packs().contains("fr"));
}

// 5.10 - Test toProto with empty collections
TEST_F(DeviceTest, Device_ToProtoWithEmptyCollections) {
    // Create a device with no additional items
    auto emptyDevice = std::make_unique<Device>(
        3,  // slot
        catena::Device_DetailLevel_FULL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    catena::Device proto;
    emptyDevice->toProto(proto, *adminAuthz_, false);
    
    // Verify basic properties are serialized
    EXPECT_EQ(proto.slot(), 3);
    EXPECT_EQ(proto.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_TRUE(proto.multi_set_enabled());
    EXPECT_TRUE(proto.subscriptions());
    EXPECT_EQ(proto.default_scope(), "admin");
    
    // Verify collections are empty
    EXPECT_EQ(proto.params_size(), 0);
    EXPECT_EQ(proto.commands_size(), 0);
    EXPECT_EQ(proto.constraints_size(), 0);
    EXPECT_EQ(proto.menu_groups_size(), 0);
    EXPECT_EQ(proto.language_packs().packs_size(), 0);
}

// 5.11 - Test toProto with exception handling in parameter serialization
TEST_F(DeviceTest, Device_ToProtoWithParameterSerializationException) {
    // Create a mock parameter that throws during toProto
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/exceptionParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
    EXPECT_CALL(*mockParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) -> catena::exception_with_status {
            throw std::runtime_error("Parameter serialization failed");
        }));
    
    device_->addItem("exceptionParam", mockParam.get());
    
    // Should throw because Device::toProto doesn't catch exceptions
    catena::Device proto;
    EXPECT_THROW(device_->toProto(proto, *adminAuthz_, false), std::runtime_error);
}

// 5.12 - Test toProto with exception handling in constraint serialization
TEST_F(DeviceTest, Device_ToProtoWithConstraintSerializationException) {
    // Create a mock constraint that throws during toProto
    auto mockConstraint = std::make_shared<MockConstraint>();
    
    EXPECT_CALL(*mockConstraint, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            throw std::runtime_error("Constraint serialization failed");
        }));
    
    device_->addItem("exceptionConstraint", mockConstraint.get());
    
    // Should throw because Device::toProto doesn't catch exceptions
    catena::Device proto;
    EXPECT_THROW(device_->toProto(proto, *adminAuthz_, false), std::runtime_error);
}

// 5.13 - Test toProto with exception handling in menu group serialization
TEST_F(DeviceTest, Device_ToProtoWithMenuGroupSerializationException) {
    // Create a mock menu group that throws during toProto
    auto mockMenuGroup = std::make_shared<MockMenuGroup>();
    
    EXPECT_CALL(*mockMenuGroup, toProto(testing::An<catena::MenuGroup&>(), false))
        .WillOnce(testing::Invoke([](catena::MenuGroup& menuGroup, bool shallow) {
            throw std::runtime_error("Menu group serialization failed");
        }));
    
    device_->addItem("exceptionMenuGroup", mockMenuGroup.get());
    
    // Should throw because Device::toProto doesn't catch exceptions
    catena::Device proto;
    EXPECT_THROW(device_->toProto(proto, *adminAuthz_, false), std::runtime_error);
}

// 5.14 - Test toProto with different detail levels
TEST_F(DeviceTest, Device_ToProtoWithDifferentDetailLevels) {
    // Test with NONE detail level
    auto noneDevice = std::make_unique<Device>(
        4,  // slot
        catena::Device_DetailLevel_NONE,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    catena::Device protoNone;
    noneDevice->toProto(protoNone, *adminAuthz_, false);
    
    EXPECT_EQ(protoNone.slot(), 4);
    EXPECT_EQ(protoNone.detail_level(), catena::Device_DetailLevel_NONE);
    EXPECT_EQ(protoNone.params_size(), 0);
    EXPECT_EQ(protoNone.commands_size(), 0);
    EXPECT_EQ(protoNone.constraints_size(), 0);
    EXPECT_EQ(protoNone.menu_groups_size(), 0);
    
    // Test with FULL detail level (should include everything)
    auto fullDevice = std::make_unique<Device>(
        5,  // slot
        catena::Device_DetailLevel_FULL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Add some items to test serialization
    auto mockParam = std::make_shared<MockParam>();
    auto mockConstraint = std::make_shared<MockConstraint>();
    auto mockMenuGroup = std::make_shared<MockMenuGroup>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
    EXPECT_CALL(*mockParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockConstraint, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            constraint.set_ref_oid("testConstraint");
        }));
    EXPECT_CALL(*mockMenuGroup, toProto(testing::An<catena::MenuGroup&>(), false))
        .WillOnce(testing::Invoke([](catena::MenuGroup& menuGroup, bool shallow) {
            auto* name = menuGroup.mutable_name();
            name->mutable_display_strings()->insert({"en", "Test Menu Group"});
        }));
    
    fullDevice->addItem("testParam", mockParam.get());
    fullDevice->addItem("testConstraint", mockConstraint.get());
    fullDevice->addItem("testMenuGroup", mockMenuGroup.get());
    
    catena::Device protoFull;
    fullDevice->toProto(protoFull, *adminAuthz_, false);
    
    EXPECT_EQ(protoFull.slot(), 5);
    EXPECT_EQ(protoFull.detail_level(), catena::Device_DetailLevel_FULL);
    EXPECT_EQ(protoFull.params_size(), 1);
    EXPECT_EQ(protoFull.constraints_size(), 1);
    EXPECT_EQ(protoFull.menu_groups_size(), 1);
}

// --- toProto Language Tests ---

// ==== 6. Device Serializer Tests ====



// ==== 7. shouldSendParam Tests ====

// 7.1 - Test shouldSendParam with FULL detail level
TEST_F(DeviceTest, ShouldSendParam_FullDetailLevel) {
    // Create a device with FULL detail level
    auto fullDevice = std::make_unique<Device>(
        6,  // slot
        catena::Device_DetailLevel_FULL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockMinimalParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockMinimalDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockCommandDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockMinimalParam.get(), "/minimalParam", mockMinimalDescriptor.get(), false, 0, adminScope);
    
    // Set up expectations for descriptors
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockMinimalDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(true));
    
    // Test that all parameters should be sent in FULL detail level
    EXPECT_TRUE(fullDevice->shouldSendParam(*mockParam, false, *adminAuthz_));
    EXPECT_TRUE(fullDevice->shouldSendParam(*mockCommand, false, *adminAuthz_));
    EXPECT_TRUE(fullDevice->shouldSendParam(*mockMinimalParam, false, *adminAuthz_));
}

// 7.2 - Test shouldSendParam with COMMANDS detail level
TEST_F(DeviceTest, ShouldSendParam_CommandsDetailLevel) {
    // Create a device with COMMANDS detail level
    auto commandsDevice = std::make_unique<Device>(
        7,  // slot
        catena::Device_DetailLevel_COMMANDS,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockMinimalParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockMinimalDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockCommandDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockMinimalParam.get(), "/minimalParam", mockMinimalDescriptor.get(), false, 0, adminScope);
    
    // Set up expectations for descriptors
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockMinimalDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(true));
    
    // Test that only commands should be sent in COMMANDS detail level
    EXPECT_FALSE(commandsDevice->shouldSendParam(*mockParam, false, *adminAuthz_));
    EXPECT_TRUE(commandsDevice->shouldSendParam(*mockCommand, false, *adminAuthz_));
    EXPECT_FALSE(commandsDevice->shouldSendParam(*mockMinimalParam, false, *adminAuthz_));
}

// 7.3 - Test shouldSendParam with MINIMAL detail level
TEST_F(DeviceTest, ShouldSendParam_MinimalDetailLevel) {
    // Create a device with MINIMAL detail level
    auto minimalDevice = std::make_unique<Device>(
        8,  // slot
        catena::Device_DetailLevel_MINIMAL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockMinimalParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockMinimalDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Set up expectations for descriptors BEFORE setupMockParam to avoid conflicts
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockMinimalDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(true));
    
    // Set up mock parameters after descriptor expectations
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockCommandDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockMinimalParam.get(), "/minimalParam", mockMinimalDescriptor.get(), false, 0, adminScope);
    
    // Test that only minimal parameters should be sent in MINIMAL detail level
    EXPECT_FALSE(minimalDevice->shouldSendParam(*mockParam, false, *adminAuthz_));
    EXPECT_FALSE(minimalDevice->shouldSendParam(*mockCommand, false, *adminAuthz_));
    EXPECT_TRUE(minimalDevice->shouldSendParam(*mockMinimalParam, false, *adminAuthz_));
}

// 7.4 - Test shouldSendParam with SUBSCRIPTIONS detail level
TEST_F(DeviceTest, ShouldSendParam_SubscriptionsDetailLevel) {
    // Create a device with SUBSCRIPTIONS detail level
    auto subscriptionsDevice = std::make_unique<Device>(
        9,  // slot
        catena::Device_DetailLevel_SUBSCRIPTIONS,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockMinimalParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockMinimalDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Set up expectations for descriptors BEFORE setupMockParam to avoid conflicts
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockMinimalDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(true));
    
    // Set up mock parameters after descriptor expectations
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockCommandDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockMinimalParam.get(), "/minimalParam", mockMinimalDescriptor.get(), false, 0, adminScope);
    
    // Test that minimal parameters and subscribed parameters should be sent in SUBSCRIPTIONS detail level
    EXPECT_FALSE(subscriptionsDevice->shouldSendParam(*mockParam, false, *adminAuthz_)); // Not minimal, not subscribed
    EXPECT_FALSE(subscriptionsDevice->shouldSendParam(*mockCommand, false, *adminAuthz_)); // Not minimal, not subscribed
    EXPECT_TRUE(subscriptionsDevice->shouldSendParam(*mockMinimalParam, false, *adminAuthz_)); // Minimal
    EXPECT_TRUE(subscriptionsDevice->shouldSendParam(*mockParam, true, *adminAuthz_)); // Subscribed
    EXPECT_TRUE(subscriptionsDevice->shouldSendParam(*mockCommand, true, *adminAuthz_)); // Subscribed
}

// 7.5 - Test shouldSendParam with NONE detail level
TEST_F(DeviceTest, ShouldSendParam_NoneDetailLevel) {
    // Create a device with NONE detail level
    auto noneDevice = std::make_unique<Device>(
        10,  // slot
        catena::Device_DetailLevel_NONE,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockMinimalParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockMinimalDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Set up expectations for descriptors BEFORE setupMockParam to avoid conflicts
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockMinimalDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(true));
    
    // Set up mock parameters after descriptor expectations
    setupMockParam(mockParam.get(), "/testParam", mockDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockCommand.get(), "/testCommand", mockCommandDescriptor.get(), false, 0, adminScope);
    setupMockParam(mockMinimalParam.get(), "/minimalParam", mockMinimalDescriptor.get(), false, 0, adminScope);
    
    // Test that all parameters should be sent in NONE detail level (if authorized)
    EXPECT_TRUE(noneDevice->shouldSendParam(*mockParam, false, *adminAuthz_));
    EXPECT_TRUE(noneDevice->shouldSendParam(*mockCommand, false, *adminAuthz_));
    EXPECT_TRUE(noneDevice->shouldSendParam(*mockMinimalParam, false, *adminAuthz_));
}

// // 5.20 - Test shouldSendParam with authorization filtering
// TEST_F(DeviceTest, ShouldSendParam_AuthorizationFiltering) {
//     // Create a device with FULL detail level
//     auto device = std::make_unique<Device>(
//         11,  // slot
//         catena::Device_DetailLevel_FULL,  // detail_level
//         std::vector<std::string>{"admin"},  // access_scopes
//         "admin",  // default_scope
//         true,  // multi_set_enabled
//         true   // subscriptions
//     );
    
//     // Create mock parameters with different authorization requirements
//     auto mockAuthorizedParam = std::make_shared<MockParam>();
//     auto mockUnauthorizedParam = std::make_shared<MockParam>();
//     auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
//     auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
//     // Set up authorization - authorized param allows monitor access, unauthorized param requires admin
//     static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
//     static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
//     // Set up expectations for descriptors
//     EXPECT_CALL(*mockDescriptor1, minimalSet())
//         .WillRepeatedly(testing::Return(false));
//     EXPECT_CALL(*mockDescriptor2, minimalSet())
//         .WillRepeatedly(testing::Return(false));
    
//     // Set up expectations for getScope to ensure authorization works correctly
//     EXPECT_CALL(*mockAuthorizedParam, getScope())
//         .WillRepeatedly(testing::ReturnRef(monitorScope));
//     EXPECT_CALL(*mockUnauthorizedParam, getScope())
//         .WillRepeatedly(testing::ReturnRef(adminScope));
    
//     // Set up other mock expectations manually to avoid conflicts with getScope
//     static const std::string authorizedOid = "/authorizedParam";
//     static const std::string unauthorizedOid = "/unauthorizedParam";
    
//     EXPECT_CALL(*mockAuthorizedParam, getOid())
//         .WillRepeatedly(testing::ReturnRef(authorizedOid));
//     EXPECT_CALL(*mockAuthorizedParam, getDescriptor())
//         .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
//     EXPECT_CALL(*mockAuthorizedParam, isArrayType())
//         .WillRepeatedly(testing::Return(false));
    
//     EXPECT_CALL(*mockUnauthorizedParam, getOid())
//         .WillRepeatedly(testing::ReturnRef(unauthorizedOid));
//     EXPECT_CALL(*mockUnauthorizedParam, getDescriptor())
//         .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
//     EXPECT_CALL(*mockUnauthorizedParam, isArrayType())
//         .WillRepeatedly(testing::Return(false));
    
//     // Test with monitor authorization (should only allow authorized param)
//     EXPECT_TRUE(device->shouldSendParam(*mockAuthorizedParam, false, *monitorAuthz_));
//     EXPECT_FALSE(device->shouldSendParam(*mockUnauthorizedParam, false, *monitorAuthz_));
    
//     // Test with admin authorization (should allow both)
//     EXPECT_TRUE(device->shouldSendParam(*mockAuthorizedParam, false, *adminAuthz_));
//     EXPECT_TRUE(device->shouldSendParam(*mockUnauthorizedParam, false, *adminAuthz_));
// }

// // 5.21 - Test shouldSendParam with UNSET detail level
// TEST_F(DeviceTest, ShouldSendParam_UnsetDetailLevel) {
//     // Create a device with UNSET detail level
//     auto unsetDevice = std::make_unique<Device>(
//         12,  // slot
//         catena::Device_DetailLevel_UNSET,  // detail_level
//         std::vector<std::string>{"admin"},  // access_scopes
//         "admin",  // default_scope
//         true,  // multi_set_enabled
//         true   // subscriptions
//     );
    
//     // Create mock parameter
//     auto mockParam = std::make_shared<MockParam>();
//     auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
//     // Set up authorization - admin token has st2138:adm:w scope
//     static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
//     // Set up expectations for descriptor
//     EXPECT_CALL(*mockDescriptor, minimalSet())
//         .WillRepeatedly(testing::Return(false));
    
//     // Set up expectations for getScope to ensure authorization works correctly
//     EXPECT_CALL(*mockParam, getScope())
//         .WillRepeatedly(testing::ReturnRef(adminScope));
    
//     // Set up other mock expectations manually to avoid conflicts with getScope
//     static const std::string testOid = "/testParam";
    
//     EXPECT_CALL(*mockParam, getOid())
//         .WillRepeatedly(testing::ReturnRef(testOid));
//     EXPECT_CALL(*mockParam, getDescriptor())
//         .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
//     EXPECT_CALL(*mockParam, isArrayType())
//         .WillRepeatedly(testing::Return(false));
    
//     // Test that parameters should be sent in UNSET detail level (treated as FULL)
//     EXPECT_TRUE(unsetDevice->shouldSendParam(*mockParam, false, *adminAuthz_));
// }