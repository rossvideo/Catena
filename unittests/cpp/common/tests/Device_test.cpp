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
#include <mocks/MockMenu.h>
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
        
        // Add a minimal set parameter to the device (always included regardless of subscription)
        auto minimalSetParam = std::make_shared<MockParam>();
        auto minimalSetDescriptor = std::make_shared<MockParamDescriptor>();
        mockDescriptors_.push_back(minimalSetDescriptor);
        mockParams_.push_back(minimalSetParam);
        
        static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
        setupMockParam(*minimalSetParam, "/minimalSetParam", *minimalSetDescriptor, false, 0, monitorScope);
        EXPECT_CALL(*minimalSetDescriptor, minimalSet())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*minimalSetParam, getDescriptor())
            .WillRepeatedly(testing::ReturnRef(*minimalSetDescriptor));
        EXPECT_CALL(*minimalSetParam, toProto(testing::An<catena::Param&>(), testing::_))
            .WillRepeatedly(testing::Invoke([](catena::Param& param, Authorizer& authz) {
                param.set_type(catena::ParamType::INT32);
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
        device_->addItem("minimalSetParam", minimalSetParam.get());

        // Using the admin/monitor tokens from CommonTestHelpers.h
        std::string adminToken = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kAdmin) + ":w");
        adminAuthz_ = std::make_unique<Authorizer>(adminToken);
        std::string monitorToken = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));
        monitorAuthz_ = std::make_unique<Authorizer>(monitorToken);
    }

    std::unique_ptr<Device> device_;
    std::unique_ptr<Authorizer> adminAuthz_;
    std::unique_ptr<Authorizer> monitorAuthz_;
    std::shared_ptr<LanguagePack> englishPack_;
    std::shared_ptr<LanguagePack> frenchPack_;
    
    // Store params and descriptors to keep them alive
    std::vector<std::shared_ptr<MockParam>> mockParams_;
    std::vector<std::shared_ptr<MockParamDescriptor>> mockDescriptors_;
 
    // Helper function for multi-set value tests
    std::shared_ptr<MockParam> createMultiSetMockParam(const std::string& oid, const std::string& errorMsg = "") {
        auto mockParam = std::make_shared<MockParam>();
        auto mockDescriptor = std::make_shared<MockParamDescriptor>();
        mockDescriptors_.push_back(mockDescriptor);
        mockParams_.push_back(mockParam);
        
        static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
        setupMockParam(*mockParam, oid, *mockDescriptor, false, 0, adminScope);
        
        EXPECT_CALL(*mockParam, getDescriptor())
            .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
        EXPECT_CALL(*mockParam, copy())
            .WillOnce(testing::Invoke([errorMsg]() { 
                auto mock = std::make_unique<MockParam>();
                if (errorMsg.empty()) {
                    // Success case - no error message
                    EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                        .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                            status = catena::exception_with_status("", catena::StatusCode::OK);
                            return true;
                        }));
                } else if (errorMsg.find("exception") != std::string::npos) {
                    // Exception case - throw during copy
                    throw catena::exception_with_status(errorMsg, catena::StatusCode::INTERNAL);
                } else {
                    // Validation failure case - return false with error message
                    EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                        .WillOnce(testing::Invoke([errorMsg](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                            status = catena::exception_with_status(errorMsg, catena::StatusCode::INVALID_ARGUMENT);
                            return false;
                        }));
                }
                return mock;
            }))
            .WillOnce(testing::Invoke([]() { 
                auto mock = std::make_unique<MockParam>();
                EXPECT_CALL(*mock, resetValidate())
                    .Times(1);
                return mock;
            }));
        
        return mockParam;
    }

    std::shared_ptr<MockParam> createGetValueMockParam(const std::string& oid, const std::string& returnValue = "", int returnInt = 0, const std::string& errorMsg = "", bool throwException = false) {
        auto mockParam = std::make_shared<MockParam>();
        auto mockDescriptor = std::make_shared<MockParamDescriptor>();
        mockDescriptors_.push_back(mockDescriptor);
        mockParams_.push_back(mockParam);
        
        static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
        setupMockParam(*mockParam, oid, *mockDescriptor, false, 0, adminScope);
        
        EXPECT_CALL(*mockParam, getDescriptor())
            .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
        EXPECT_CALL(*mockParam, copy())
            .WillRepeatedly(testing::Invoke([returnValue, returnInt, errorMsg, throwException]() { 
                auto copiedMock = std::make_unique<MockParam>();
                
                if (errorMsg.empty()) {
                    // Success case
                    EXPECT_CALL(*copiedMock, toProto(testing::Matcher<catena::Value&>(testing::_), testing::_))
                        .WillOnce(testing::Invoke([returnValue, returnInt](catena::Value& dst, const catena::common::Authorizer&) {
                            if (!returnValue.empty()) {
                                dst.set_string_value(returnValue);
                            } else {
                                dst.set_int32_value(returnInt);
                            }
                            return catena::exception_with_status("", catena::StatusCode::OK);
                        }));
                } else if (throwException) {
                    // Exception case
                    EXPECT_CALL(*copiedMock, toProto(testing::Matcher<catena::Value&>(testing::_), testing::_))
                        .WillOnce(testing::Throw(std::runtime_error(errorMsg)));
                } else {
                    // Error case
                    EXPECT_CALL(*copiedMock, toProto(testing::Matcher<catena::Value&>(testing::_), testing::_))
                        .WillOnce(testing::Invoke([errorMsg](catena::Value&, const catena::common::Authorizer&) {
                            return catena::exception_with_status(errorMsg, catena::StatusCode::PERMISSION_DENIED);
                        }));
                }
                
                return copiedMock;
            }));
        
        return mockParam;
    }

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

// 1.1: Success Case - Multi-Set Value with Single Value
TEST_F(DeviceTest, TryMultiSetValue_SingleValue) {
    auto mockParam = createMultiSetMockParam("/param1");
    device_->addItem("param1", mockParam.get());
    
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.2: Success Case - Multi-Set Value with Multiple Parameters
TEST_F(DeviceTest, TryMultiSetValue_MultipleParams) {
    auto mockParam1 = createMultiSetMockParam("/param1");
    auto mockParam2 = createMultiSetMockParam("/param2");
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
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
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.3: Error Case - Multi-Set Value with Multi-Set Disabled
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
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = deviceDisabled->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(status.what()), "Multi-set is disabled for the device in slot 1");
}

// 1.4: Error Case - Multi-Set Value with Overlapping OIDs
TEST_F(DeviceTest, TryMultiSetValue_OverlappingOids) {
    auto mockParam1 = createMultiSetMockParam("/param1");
    device_->addItem("param1", mockParam1.get());
    
    catena::MultiSetValuePayload payload;
    
    // First value - parent path
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value - same path (exact overlap)
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param1");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Overlapping actions for /param1 and /param1");
}

// 1.5: Error Case - Multi-Set Value with Catena Exception
TEST_F(DeviceTest, TryMultiSetValue_CatenaException) {
    auto mockParam = createMultiSetMockParam("/param1", "Test catena exception");
    device_->addItem("param1", mockParam.get());
    
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Test catena exception");
}

// 1.6: Error Case - Multi-Set Value with Empty Payload
TEST_F(DeviceTest, TryMultiSetValue_EmptyPayload) {
    catena::MultiSetValuePayload payload;
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 1.7: Error Case - Multi-Set Value with Authorization Failure
TEST_F(DeviceTest, TryMultiSetValue_AuthFailure) {
    // Create mock parameter manually
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    mockParams_.push_back(mockParam);
    mockDescriptors_.push_back(mockDescriptor);
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/param1", *mockDescriptor, false, 0, adminScope);
    
    // Set up getDescriptor expectation
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));

    device_->addItem("param1", mockParam.get());
    
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *monitorAuthz_);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
}

// 1.8: Error Case - Multi-Set Value with Validation Failure
TEST_F(DeviceTest, TryMultiSetValue_ValidationFailure) {
    auto mockParam1 = createMultiSetMockParam("/param1");
    auto mockParam2 = createMultiSetMockParam("/param2", "Validation failed");
    
    device_->addItem("param1", mockParam1.get());
    device_->addItem("param2", mockParam2.get());
    
    catena::MultiSetValuePayload payload;
    
    // First value - should validate successfully
    auto* setValue1 = payload.add_values();
    setValue1->set_oid("/param1");
    auto* value1 = setValue1->mutable_value();
    value1->set_int32_value(42);
    
    // Second value - should fail validation (causing entire operation to fail)
    auto* setValue2 = payload.add_values();
    setValue2->set_oid("/param2");
    auto* value2 = setValue2->mutable_value();
    value2->set_string_value("test");
    
    catena::exception_with_status status{"", catena::StatusCode::OK};
    bool result = device_->tryMultiSetValue(payload, status, *adminAuthz_);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Validation failed");
}

// --- commitMultiSetValue Tests ---

// 1.9: Success Case - Test commitMultiSetValue with single value
TEST_F(DeviceTest, CommitMultiSetValue_SingleValue) {
    // Create mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/param1", *mockDescriptor, false, 0, adminScope);
    
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

// 1.10: Success Case - Test commitMultiSetValue with regular parameters
TEST_F(DeviceTest, CommitMultiSetValue_RegularParams) {
    // Create parameter hierarchy using the helper
    auto regularParam1 = ParamHierarchyBuilder::createDescriptor("/param1");
    auto regularParam2 = ParamHierarchyBuilder::createDescriptor("/param2");
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Create mock parameters with proper descriptors
    auto mockRegularParam1 = std::make_shared<MockParam>();
    auto mockRegularParam2 = std::make_shared<MockParam>();
    
    setupMockParam(*mockRegularParam1, "/param1", *regularParam1.descriptor, false, 0, adminScope);
    setupMockParam(*mockRegularParam2, "/param2", *regularParam2.descriptor, false, 0, adminScope);
    
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
    auto signalConnection = device_->getValueSetByClient().connect(
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

// 1.11: Success Case - Test commitMultiSetValue with array append operation (addBack)
TEST_F(DeviceTest, CommitMultiSetValue_ArrayAppend) {
    // Create parameter hierarchy using the helper
    auto arrayParam = ParamHierarchyBuilder::createDescriptor("/arrayParam");
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Create mock parameters with proper descriptors
    auto mockArrayParam = std::make_shared<MockParam>();
    mockParams_.push_back(mockArrayParam);
    
    // Set up array parameter (isArray = true, size = 5)
    setupMockParam(*mockArrayParam, "/arrayParam", *arrayParam.descriptor, true, 5, adminScope);
    
    // Set up expectations for array parameter copy
    EXPECT_CALL(*mockArrayParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Set up expectations for the copied mock to handle array append operation
            EXPECT_CALL(*mock, addBack(testing::_, testing::_))
                .WillOnce(testing::Invoke([](catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    auto appendedMock = std::make_unique<MockParam>();
                    EXPECT_CALL(*appendedMock, fromProto(testing::_, testing::_))
                        .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
                            return catena::exception_with_status("", catena::StatusCode::OK);
                        }));
                    return std::unique_ptr<IParam>(std::move(appendedMock));
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("arrayParam", mockArrayParam.get());
    
    // Track signal emissions
    std::vector<std::pair<std::string, const IParam*>> signalEmissions;
    auto signalConnection = device_->getValueSetByClient().connect(
        [&signalEmissions](const std::string& oid, const IParam* param) {
            signalEmissions.emplace_back(oid, param);
        }
    );
    
    // Create a payload with array append operation
    catena::MultiSetValuePayload payload;
    
    // Value - append new element to array
    auto* setValue = payload.add_values();
    setValue->set_oid("/arrayParam/-");
    auto* value = setValue->mutable_value();
    value->set_string_value("appended");
    
    // Test the commit
    catena::exception_with_status status{"", catena::StatusCode::OK};
    status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
    // Should succeed
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    
    // Should have emitted one signal for the array append operation
    EXPECT_EQ(signalEmissions.size(), 1);
    
    // Check signal emission
    EXPECT_EQ(signalEmissions[0].first, "/arrayParam/-");
    EXPECT_NE(signalEmissions[0].second, nullptr);
}

// 1.12: Success Case - Test commitMultiSetValue with array parameters (indexed access)
TEST_F(DeviceTest, CommitMultiSetValue_ArrayParams) {
    // Create parameter hierarchy using the helper
    auto arrayParam = ParamHierarchyBuilder::createDescriptor("/arrayParam");
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    
    // Create mock parameters with proper descriptors
    auto mockArrayParam = std::make_shared<MockParam>();
    auto mockElementParam = std::make_shared<MockParam>();
    mockParams_.push_back(mockArrayParam);
    mockParams_.push_back(mockElementParam);
    
    // Set up array parameter (isArray = true, size = 5)
    setupMockParam(*mockArrayParam, "/arrayParam", *arrayParam.descriptor, true, 5, adminScope);
    setupMockParam(*mockElementParam, "/arrayParam/3", *arrayParam.descriptor, false, 0, adminScope);
    
    // Set up expectations for array parameter copy
    EXPECT_CALL(*mockArrayParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Set up expectations for the copied mock to handle array element access
            EXPECT_CALL(*mock, getParam(testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](catena::common::Path& path, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    auto elementMock = std::make_unique<MockParam>();
                    EXPECT_CALL(*elementMock, fromProto(testing::_, testing::_))
                        .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Authorizer&) {
                            return catena::exception_with_status("", catena::StatusCode::OK);
                        }));
                    return std::unique_ptr<IParam>(std::move(elementMock));
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));
    
    device_->addItem("arrayParam", mockArrayParam.get());
    
    // Track signal emissions
    std::vector<std::pair<std::string, const IParam*>> signalEmissions;
    auto signalConnection = device_->getValueSetByClient().connect(
        [&signalEmissions](const std::string& oid, const IParam* param) {
            signalEmissions.emplace_back(oid, param);
        }
    );
    
    // Create a payload with array indexed access
    catena::MultiSetValuePayload payload;
    
    // Value - indexed array element
    auto* setValue = payload.add_values();
    setValue->set_oid("/arrayParam/3");
    auto* value = setValue->mutable_value();
    value->set_int32_value(100);
    
    // Test the commit
    catena::exception_with_status status{"", catena::StatusCode::OK};
    status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
    // Should succeed
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    
    // Should have emitted one signal for the array indexed access
    EXPECT_EQ(signalEmissions.size(), 1);
    
    // Check signal emission
    EXPECT_EQ(signalEmissions[0].first, "/arrayParam/3");
    EXPECT_NE(signalEmissions[0].second, nullptr);
}

// 1.13: Error Case - Test commitMultiSetValue with Catena exception
TEST_F(DeviceTest, CommitMultiSetValue_CatenaException) {
    // Create mock parameter
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    mockParams_.push_back(mockParam);
    mockDescriptors_.push_back(mockDescriptor);
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/param1", *mockDescriptor, false, 0, adminScope);
    
    // Set up expectations for copy to throw exception
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() -> std::unique_ptr<catena::common::IParam> { 
            throw catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
        }));
    
    device_->addItem("param1", mockParam.get());
    
    catena::MultiSetValuePayload payload;
    auto* setValue = payload.add_values();
    setValue->set_oid("/param1");
    auto* value = setValue->mutable_value();
    value->set_int32_value(42);
    
    // Test the commit - should throw an exception
    catena::exception_with_status status{"", catena::StatusCode::OK};
    status = device_->commitMultiSetValue(payload, *adminAuthz_);
    
    // Should fail with internal error
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Test catena exception");
}

// ======== 2. Set/Get Value Tests ========

// --- Set Value Tests ---

// 2.1: Success Case - Set Value String
TEST_F(DeviceTest, SetValue_String) {
    // Create a mock parameter that accepts string values
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    mockDescriptors_.push_back(mockDescriptor);
    mockParams_.push_back(mockParam);
    
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/setParam", *mockDescriptor, false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));

    // Setup 3 copies of the mock param: validation, reset, commit
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() {        
            auto mock = std::make_unique<MockParam>();
            // First copy is for validation - expect validateSetValue
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Second copy is for reset in tryMultiSetValue cleanup - expect resetValidate
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Third copy is for commit - expect fromProto and resetValidate
            EXPECT_CALL(*mock, fromProto(testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, const catena::common::Authorizer&) {
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));

    device_->addItem("setParam", mockParam.get());

    // Create value to set
    catena::Value value;
    value.set_string_value("new_value");

    // Test setting the value
    auto status = device_->setValue("/setParam", value, *adminAuthz_);

    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 2.2: Success Case - Set Value Integer
TEST_F(DeviceTest, SetValue_Integer) {
    // Create a mock parameter that accepts integer values
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    mockDescriptors_.push_back(mockDescriptor);
    mockParams_.push_back(mockParam);
    
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/intSetParam", *mockDescriptor, false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));

    // Setup 3 copies of the mock param: validation, reset, commit
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // First copy is for validation - expect validateSetValue
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Second copy is for reset in tryMultiSetValue cleanup - expect resetValidate
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Third copy is for commit - expect fromProto and resetValidate
            EXPECT_CALL(*mock, fromProto(testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, const catena::common::Authorizer&) {
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));

    device_->addItem("intSetParam", mockParam.get());
    
    // Create value to set
    catena::Value value;
    value.set_int32_value(100);
    
    // Test setting the value
    auto status = device_->setValue("/intSetParam", value, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
}

// 2.3: Error Case - Set Value Validation Failed
TEST_F(DeviceTest, SetValue_ValidationFailed) {
    // Create a mock parameter that fails validation
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    mockDescriptors_.push_back(mockDescriptor);
    mockParams_.push_back(mockParam);
    
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/invalidParam", *mockDescriptor, false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));

    // Setup 2 copies of the mock param: validation, reset
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // First copy is for validation - expect validateSetValue to fail
            EXPECT_CALL(*mock, validateSetValue(testing::_, testing::_, testing::_, testing::_))
                .WillOnce(testing::Invoke([](const catena::Value&, catena::common::Path::Index, catena::common::Authorizer&, catena::exception_with_status& status) {
                    status = catena::exception_with_status("Validation failed", catena::StatusCode::INVALID_ARGUMENT);
                    return false;
                }));
            return mock;
        }))
        .WillOnce(testing::Invoke([]() { 
            auto mock = std::make_unique<MockParam>();
            // Second copy is for reset - expect resetValidate
            EXPECT_CALL(*mock, resetValidate())
                .Times(1);
            return mock;
        }));

    device_->addItem("invalidParam", mockParam.get());
    
    // Create value to set
    catena::Value value;
    value.set_string_value("invalid_value");
    
    // Test setting the value - should fail validation
    auto status = device_->setValue("/invalidParam", value, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Validation failed");
}

// --- Get Value Tests ---

// 2.4: Success Case - Get Value String
TEST_F(DeviceTest, GetValue_String) {
    // Create a mock parameter that returns a string value
    auto mockParam = createGetValueMockParam("/testParam", "test_value");

    device_->addItem("testParam", mockParam.get());

    // Test getting the value
    catena::Value result;
    auto status = device_->getValue("/testParam", result, *adminAuthz_);

    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.string_value(), "test_value");
}

// 2.5: Success Case - Get Value Integer
TEST_F(DeviceTest, GetValue_Integer) {
    // Create a mock parameter that returns an integer value
    auto mockParam = createGetValueMockParam("/intParam", "", 42);
    device_->addItem("intParam", mockParam.get());

    // Test getting the value
    catena::Value result;
    auto status = device_->getValue("/intParam", result, *adminAuthz_);

    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.int32_value(), 42);
}

// 2.6: Error Case - Get Value Invalid Json Pointer
TEST_F(DeviceTest, GetValue_InvalidJsonPointer) {
    catena::Value result;
    auto status = device_->getValue("invalid/pointer", result, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
}

// 2.7: Error Case - Get Value Path Not Found
TEST_F(DeviceTest, GetValue_PathNotFound) {
    catena::Value result;
    auto status = device_->getValue("/nonexistent", result, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Param /nonexistent does not exist");
}

// 2.8: Error Case - Get Value Not Authorized
TEST_F(DeviceTest, GetValue_NotAuthorized) {
    auto mockParam = createGetValueMockParam("/authParam", "", 0, "Not authorized");
    device_->addItem("authParam", mockParam.get());
    catena::Value result;
    auto status = device_->getValue("/authParam", result, *monitorAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(status.what()), "Not authorized to read the param /authParam");
}

// 2.9: Error Case - Get Value Index Out of Bounds
TEST_F(DeviceTest, GetValue_IndexOutOfBounds) {
    catena::Value result;
    auto status = device_->getValue("/param/-", result, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OUT_OF_RANGE);
    EXPECT_EQ(std::string(status.what()), "Index out - of bounds in path /param/-");
}

// 2.10: Error Case - Get Value Internal Exception
TEST_F(DeviceTest, GetValue_InternalException) {
    // Create a mock parameter that throws std::exception
    auto mockParam = createGetValueMockParam("/errorParam", "", 0, "Internal error in toProto", true);
    device_->addItem("errorParam", mockParam.get());
    
    catena::Value result;
    auto status = device_->getValue("/errorParam", result, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INTERNAL);
    EXPECT_EQ(std::string(status.what()), "Internal error in toProto");
}

// 2.11: Error Case - Get Value Unknown Exception
TEST_F(DeviceTest, GetValue_UnknownException) {
    // Create a mock parameter that throws unknown exception
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    mockDescriptors_.push_back(mockDescriptor);
    mockParams_.push_back(mockParam);
    
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/unknownParam", *mockDescriptor, false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor));
    EXPECT_CALL(*mockParam, copy())
        .WillRepeatedly(testing::Invoke([]() { 
            auto copiedMock = std::make_unique<MockParam>();
            EXPECT_CALL(*copiedMock, toProto(testing::Matcher<catena::Value&>(testing::_), testing::_))
                .WillOnce(testing::Invoke([](catena::Value&, const catena::common::Authorizer&) -> catena::exception_with_status {
                    throw 42; // Throw an int (unknown exception type)
                }));
            return copiedMock;
        }));
    
    device_->addItem("unknownParam", mockParam.get());
    
    catena::Value result;
    auto status = device_->getValue("/unknownParam", result, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::UNKNOWN);
    EXPECT_EQ(std::string(status.what()), "Unknown error");
}

// ======== 3. Language Tests ========

// --- Get Language Tests ---

// 3.1: Success Case - Language Pack Get
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

// 3.2: Error Case - Language Pack with Empty ID
TEST_F(DeviceTest, LanguagePack_Get_EmptyLanguageId) {
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("", pack);
    EXPECT_EQ(result.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(result.what()), "Language ID is empty");
}

// 3.3: Error Case - Language Pack Not Found
TEST_F(DeviceTest, LanguagePack_Get_NotFound) {
    Device::ComponentLanguagePack pack;
    auto result = device_->getLanguagePack("nonexistent", pack);
    EXPECT_EQ(result.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(result.what()), "Language pack 'nonexistent' not found");
}

// 3.4: Error Case - Language Pack Get Internal Error
TEST_F(DeviceTest, LanguagePack_Get_InternalError) {
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

// 3.5: Error Case - Language Pack Get Unknown Error
TEST_F(DeviceTest, LanguagePack_Get_UnknownError) {
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

// 3.6: Success Case - Language Pack Add
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

// 3.7: Error Case - Language Pack Add Not Authorized
TEST_F(DeviceTest, LanguagePack_Add_NotAuthorized) {
    // Try to add a language pack with monitor permissions (should fail)
    catena::AddLanguagePayload payload;
    payload.set_id("es");
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");
    
    auto result = device_->addLanguage(payload, *monitorAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Not authorized to add language");
}

// 3.8: Error Case - Language Pack Add Invalid
TEST_F(DeviceTest, LanguagePack_Add_Invalid) {
    catena::AddLanguagePayload payload;
    payload.set_id(""); // Empty ID should cause INVALID_ARGUMENT
    auto* languagePack = payload.mutable_language_pack();
    languagePack->set_name("Spanish");
    
    auto result = device_->addLanguage(payload, *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(result.what()), "Invalid language pack");
}

// 3.9: Error Case - Language Pack Add Cannot Overwrite Shipped Language
TEST_F(DeviceTest, LanguagePack_Add_NoOverwrite) {
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

// 3.10: Success Case - Language Pack Removal
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

// 3.11: Error Case - Language Pack Remove Not Authorized
TEST_F(DeviceTest, LanguagePack_Remove_NotAuthorized) {
    // Try to remove a language pack with monitor permissions (should fail)
    auto result = device_->removeLanguage("en", *monitorAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Not authorized to delete language");
}

// 3.12: Error Case - Language Pack Remove Cannot Delete Shipped Language
TEST_F(DeviceTest, LanguagePack_Remove_CannotDeleteShippedLanguage) {
    // Try to remove a shipped language pack (should fail)
    auto result = device_->removeLanguage("en", *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(std::string(result.what()), "Cannot delete language pack shipped with device");
}

// 3.13: Error Case - Language Pack Remove Not Found
TEST_F(DeviceTest, LanguagePack_Remove_NotFound) {
    // Try to remove a language pack that doesn't exist
    auto result = device_->removeLanguage("nonexistent", *adminAuthz_);
    EXPECT_EQ(result.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(result.what()), "Language pack 'nonexistent' not found");
}

// ======== 4. Param/Command Tests ========

// 4.1: Success Case - Get Param with Valid String Path
TEST_F(DeviceTest, GetParam_StringSuccess) {
    // Create a mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/testParam", *mockDescriptor, false, 0, adminScope);
    
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    
    device_->addItem("testParam", mockParam.get());
    
    // Test getting the parameter
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getParam("/testParam", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_NE(result, nullptr);
}

// 4.2: Success Case - Get Param with Valid Path Object
TEST_F(DeviceTest, GetParam_PathSuccess) {
    // Create a mock parameter and add it to the device
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/testParam", *mockDescriptor, false, 0, adminScope);
    
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

// 4.3: Success Case - Get Param with Sub-path
TEST_F(DeviceTest, GetParam_SubPath) {
    // Create a mock parameter that supports sub-parameters
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockSubParam = std::make_unique<MockParam>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/parentParam", *mockDescriptor, false, 0, adminScope);
    
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

// 4.4: Error Case - Get Param with Invalid Json Pointer
TEST_F(DeviceTest, GetParam_InvalidJsonPointer) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path(""); // Tests both empty and invalid json pointer
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer ");
    EXPECT_EQ(result, nullptr);
}

// 4.5: Error Case - Get Param with Path Not Found
TEST_F(DeviceTest, GetParam_PathNotFound) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/invalid/path");
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Param /invalid/path does not exist");
    EXPECT_EQ(result, nullptr);
}

// 4.6: Error Case - Get Param Not Authorized
TEST_F(DeviceTest, GetParam_NotAuthorized) {
    // Create a mock parameter that requires specific authorization
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - parameter requires admin scope but monitor token only has st2138:mon
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/restrictedParam", *mockDescriptor, false, 0, adminScope);
    
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

// 4.7: Error Case - Get Param with Non-String Front Element
TEST_F(DeviceTest, GetParam_NonStringFrontElement) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    catena::common::Path path("/123"); // Path with numeric front element
    auto result = device_->getParam(path, status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer /123");
    EXPECT_EQ(result, nullptr);
}

// --- Get Top Level Params Tests ---

// 4.8: Success Case - Get Top Level Params
TEST_F(DeviceTest, GetTopLevelParams) {
    // Create a single mock parameter that requires admin authorization
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up param with admin scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/adminParam", *mockDescriptor, false, 0, adminScope);
    EXPECT_CALL(*mockParam, copy())
        .WillOnce(testing::Return(std::make_unique<MockParam>()));
    device_->addItem("adminParam", mockParam.get());
    
    // Test with monitor authorization (should get no params)
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getTopLevelParams(status, *monitorAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.size(), 0); // No params should be returned for monitor auth
    
    // Test with admin authorization (should get the param)
    status = catena::exception_with_status{"", catena::StatusCode::OK};
    result = device_->getTopLevelParams(status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::OK);
    EXPECT_EQ(result.size(), 1); // The admin param should be returned
}

// 4.9: Error Case - Get Top Level Params with Exception
TEST_F(DeviceTest, GetTopLevelParams_Exception) {
    // Create a mock parameter that throws an exception during processing
    auto mockParam = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockParam, "/exceptionParam", *mockDescriptor, false, 0, adminScope);
    
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

// 4.10: Success Case - Get Command with Valid Path
TEST_F(DeviceTest, GetCommand) {
    // Create a mock command and add it to the device
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockCommand, "/testCommand", *mockDescriptor, false, 0, adminScope);
    
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

// 4.11: Error Case - Get Command with Empty Path
TEST_F(DeviceTest, GetCommand_EmptyPath) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer");
    EXPECT_EQ(result, nullptr);
}

// 4.12: Error Case - Get Command Not Found
TEST_F(DeviceTest, GetCommand_NotFound) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/nonexistentCommand", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::NOT_FOUND);
    EXPECT_EQ(std::string(status.what()), "Command not found: /nonexistentCommand");
    EXPECT_EQ(result, nullptr);
}

// 4.13: Error Case - Get Command with Sub-commands (Unimplemented)
TEST_F(DeviceTest, GetCommand_SubCommandsUnimplemented) {
    // Create a mock command and add it to the device
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockCommand, "/testCommand", *mockDescriptor, false, 0, adminScope);
    
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

// 4.14: Error Case - Get Command with Invalid Json Pointer
TEST_F(DeviceTest, GetCommand_InvalidJsonPointer) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/invalid[", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(result, nullptr);
}

// 4.15: Error Case - Get Command with Non-String Front Element
TEST_F(DeviceTest, GetCommand_NonStringFrontElement) {
    catena::exception_with_status status{"", catena::StatusCode::OK};
    auto result = device_->getCommand("/123", status, *adminAuthz_);
    
    EXPECT_EQ(status.status, catena::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(std::string(status.what()), "Invalid json pointer");
    EXPECT_EQ(result, nullptr);
}

// 4.16: Error Case - Get Command with Exception
TEST_F(DeviceTest, GetCommand_Exception) {
    // Create a mock command that throws an exception
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockCommand, "/exceptionCommand", *mockDescriptor, false, 0, adminScope);
    
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

// --- Base Device toProto Tests ---

// 5.1 - Success Case: toProto with shallow vs deep serialization
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

// 5.2 - Success Case: toProto with parameters serialization
TEST_F(DeviceTest, Device_ToProtoParams) {
    // Create mock parameters with different authorization requirements
    auto mockAuthorizedParam = std::make_shared<MockParam>();
    auto mockUnauthorizedParam = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - authorized param allows monitor access, unauthorized param requires admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockAuthorizedParam, "/authorizedParam", *mockDescriptor1, false, 0, monitorScope);
    setupMockParam(*mockUnauthorizedParam, "/unauthorizedParam", *mockDescriptor2, false, 0, adminScope);
    
    // Set up expectations for shouldSendParam and toProto calls
    EXPECT_CALL(*mockAuthorizedParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor1));
    EXPECT_CALL(*mockUnauthorizedParam, getDescriptor())
        .WillRepeatedly(testing::ReturnRef(*mockDescriptor2));
    
    // Only the authorized param should be serialized with monitor authorization
    EXPECT_CALL(*mockAuthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockUnauthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called
    
    device_->addItem("authorizedParam", mockAuthorizedParam.get());
    device_->addItem("unauthorizedParam", mockUnauthorizedParam.get());
    
    // Test with monitor authorization - should only serialize authorized parameters
    catena::Device proto;
    device_->toProto(proto, *monitorAuthz_, false);
    
    // Verify only authorized parameters were serialized
    EXPECT_EQ(proto.params_size(), 2); // +1 for minimal set param
    EXPECT_TRUE(proto.params().contains("authorizedParam"));
    EXPECT_FALSE(proto.params().contains("unauthorizedParam"));
    EXPECT_EQ(proto.params().at("authorizedParam").type(), catena::ParamType::INT32);
}

// 5.3 - Success Case: toProto with commands serialization
TEST_F(DeviceTest, Device_ToProtoCommands) {
    // Create mock commands and add them to the device
    auto mockCommand1 = std::make_shared<MockParam>();
    auto mockCommand2 = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - admin token has st2138:adm:w scope
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockCommand1, "/command1", *mockDescriptor1, false, 0, adminScope);
    setupMockParam(*mockCommand2, "/command2", *mockDescriptor2, false, 0, adminScope);
    
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

// 5.4 - Success Case: toProto with constraints serialization
TEST_F(DeviceTest, Device_ToProtoConstraints) {
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

// 5.5 - Success Case: toProto with language packs serialization
TEST_F(DeviceTest, Device_ToProtoLanguagePacks) {
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

// 5.6 - Success Case: toProto with menu groups serialization
TEST_F(DeviceTest, Device_ToProtoMenuGroups) {
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

// 5.7 - Success Case: toProto with minimal detail level (should skip constraints, language packs, menu groups)
TEST_F(DeviceTest, Device_ToProtoMinimal) {
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

// --- toProto Language Tests ---

// 5.8 - Success Case: toProto with LanguagePacks serialization
TEST_F(DeviceTest, LanguagePacks_ToProto) {
    // Language packs are already set up in SetUp() with English and French
    catena::LanguagePacks packs;
    device_->toProto(packs);
    
    // Verify language packs were serialized correctly
    EXPECT_EQ(packs.packs_size(), 2);
    EXPECT_TRUE(packs.packs().contains("en"));
    EXPECT_TRUE(packs.packs().contains("fr"));
    
    // Verify the content of each pack
    EXPECT_EQ(packs.packs().at("en").name(), "en");
    EXPECT_EQ(packs.packs().at("fr").name(), "fr");
    
    // Verify words were copied correctly (from SetUp initialization)
    EXPECT_EQ(packs.packs().at("en").words_size(), 3); // greeting, parting, welcome
    EXPECT_EQ(packs.packs().at("fr").words_size(), 3); // greeting, parting, welcome
    
    // Verify specific words are present
    EXPECT_TRUE(packs.packs().at("en").words().contains("greeting"));
    EXPECT_TRUE(packs.packs().at("en").words().contains("parting"));
    EXPECT_TRUE(packs.packs().at("en").words().contains("welcome"));
    EXPECT_TRUE(packs.packs().at("fr").words().contains("greeting"));
    EXPECT_TRUE(packs.packs().at("fr").words().contains("parting"));
    EXPECT_TRUE(packs.packs().at("fr").words().contains("welcome"));
}

// 5.9 - Success Case: toProto with LanguageList serialization
TEST_F(DeviceTest, LanguageList_ToProto) {
    // Language packs are already set up in SetUp() with English and French
    catena::LanguageList list;
    device_->toProto(list);
    
    // Verify language list was serialized correctly
    EXPECT_EQ(list.languages_size(), 2);
    
    // Check that both languages are present (order may vary)
    bool foundEnglish = false;
    bool foundFrench = false;
    
    for (int i = 0; i < list.languages_size(); ++i) {
        if (list.languages(i) == "en") {
            foundEnglish = true;
        } else if (list.languages(i) == "fr") {
            foundFrench = true;
        }
    }
    
    EXPECT_TRUE(foundEnglish);
    EXPECT_TRUE(foundFrench);
}

// 5.10 - Success Case: toProto with language serialization (empty device)
TEST_F(DeviceTest, Language_ToProtoEmpty) {
    // Create a device with no language packs
    auto emptyDevice = std::make_unique<Device>(
        10,  // slot
        catena::Device_DetailLevel_FULL,  // detail_level
        std::vector<std::string>{"admin"},  // access_scopes
        "admin",  // default_scope
        true,  // multi_set_enabled
        true   // subscriptions
    );
    
    // Test LanguagePacks serialization with empty device
    catena::LanguagePacks packs;
    emptyDevice->toProto(packs);
    EXPECT_EQ(packs.packs_size(), 0);
    
    // Test LanguageList serialization with empty device
    catena::LanguageList list;
    emptyDevice->toProto(list);
    EXPECT_EQ(list.languages_size(), 0);
}

// ==== 6. Device Serializer Tests ====

// 6.1: Success Case - Get Device Serializer with Parameters
TEST_F(DeviceTest, GetDeviceSerializer_Parameters) {
    // Create mock parameters with different authorization requirements
    auto mockAuthorizedParam = std::make_shared<MockParam>();
    auto mockUnauthorizedParam = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - authorized param allows monitor access, unauthorized param requires admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockAuthorizedParam, "/authorizedParam", *mockDescriptor1, false, 0, monitorScope);
    setupMockParam(*mockUnauthorizedParam, "/unauthorizedParam", *mockDescriptor2, false, 0, adminScope);
    
    // Set up expectations for descriptors
    EXPECT_CALL(*mockDescriptor1, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockDescriptor2, minimalSet())
        .WillRepeatedly(testing::Return(false));
    
    // Only the authorized param should be serialized with monitor authorization
    EXPECT_CALL(*mockAuthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockUnauthorizedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called
    
    device_->addItem("authorizedParam", mockAuthorizedParam.get());
    device_->addItem("unauthorizedParam", mockUnauthorizedParam.get());
    
    // Create empty subscribed OIDs set - FULL detail level should include all components
    std::set<std::string> subscribedOids = {};
    
    // Get device serializer with monitor authorization
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundAuthorizedParam = false;
    bool foundUnauthorizedParam = false;
    bool foundMinimalSetParam = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // First component should be device info
        if (componentCount == 1) {
            EXPECT_TRUE(component.has_device());
            EXPECT_EQ(component.device().slot(), 1);
        }
        
        // Check for parameters
        if (component.has_param()) {
            if (component.param().oid() == "authorizedParam") {
                foundAuthorizedParam = true;
            } else if (component.param().oid() == "unauthorizedParam") {
                foundUnauthorizedParam = true;
            } else if (component.param().oid() == "minimalSetParam") {
                foundMinimalSetParam = true;
            }
        }
    }
    
    // Should have: device info (1) + authorized parameter (1) + minimal set param (1) + language packs from SetUp (2) = 5
    EXPECT_EQ(componentCount, 5);
    EXPECT_TRUE(foundAuthorizedParam);
    EXPECT_FALSE(foundUnauthorizedParam);
    EXPECT_TRUE(foundMinimalSetParam);
}

// 6.2: Success Case - Get Device Serializer with Commands
TEST_F(DeviceTest, GetDeviceSerializer_Commands) {
    // Create mock commands with different authorization requirements
    auto mockAuthorizedCommand = std::make_shared<MockParam>();
    auto mockUnauthorizedCommand = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - authorized command allows monitor access, unauthorized command requires admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockAuthorizedCommand, "/authorizedCommand", *mockDescriptor1, false, 0, monitorScope);
    setupMockParam(*mockUnauthorizedCommand, "/unauthorizedCommand", *mockDescriptor2, false, 0, adminScope);
    
    // Override isCommand to return true for commands
    EXPECT_CALL(*mockDescriptor1, isCommand())
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockDescriptor2, isCommand())
        .WillRepeatedly(testing::Return(true));
    
    // Only the authorized command should be serialized with monitor authorization
    EXPECT_CALL(*mockAuthorizedCommand, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockUnauthorizedCommand, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called
    
    device_->addItem("authorizedCommand", mockAuthorizedCommand.get());
    device_->addItem("unauthorizedCommand", mockUnauthorizedCommand.get());
    
    // Create empty subscribed OIDs set (not needed for COMMANDS mode)
    std::set<std::string> subscribedOids = {};
    
    // Get device serializer with COMMANDS detail level
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_COMMANDS, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundAuthorizedCommand = false;
    bool foundUnauthorizedCommand = false;
    bool foundMinimalSetParam = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // First component should be device info
        if (componentCount == 1) {
            EXPECT_TRUE(component.has_device());
            EXPECT_EQ(component.device().slot(), 1);
        }
        
        // Check for command components
        if (component.has_command()) {
            if (component.command().oid() == "authorizedCommand") {
                foundAuthorizedCommand = true;
            } else if (component.command().oid() == "unauthorizedCommand") {
                foundUnauthorizedCommand = true;
            }
        }
        
        // Check for minimal set parameter (should be included in all modes)
        if (component.has_param() && component.param().oid() == "minimalSetParam") {
            foundMinimalSetParam = true;
        }
    }
    
    // Should have: device info (1) + authorized command (1) = 2
    EXPECT_EQ(componentCount, 2);
    EXPECT_TRUE(foundAuthorizedCommand);
    EXPECT_FALSE(foundUnauthorizedCommand);
    EXPECT_FALSE(foundMinimalSetParam); // Minimal set should not be in commands mode
}

// 6.3: Success Case - Get Device Serializer with Subscriptions (+ Authorization Testing)
TEST_F(DeviceTest, GetDeviceSerializer_Subscriptions) {
    // Create mock parameters with different authorization characteristics
    auto mockAuthorizedSubscribedParam = std::make_shared<MockParam>();
    auto mockUnauthorizedSubscribedParam = std::make_shared<MockParam>();
    auto mockAuthorizedUnsubscribedParam = std::make_shared<MockParam>();
    auto mockDescriptor1 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor2 = std::make_shared<MockParamDescriptor>();
    auto mockDescriptor3 = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - authorized params allow monitor access, unauthorized params require admin
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    static const std::string adminScope = Scopes().getForwardMap().at(Scopes_e::kAdmin);
    setupMockParam(*mockAuthorizedSubscribedParam, "/authorizedSubscribedParam", *mockDescriptor1, false, 0, monitorScope);
    setupMockParam(*mockUnauthorizedSubscribedParam, "/unauthorizedSubscribedParam", *mockDescriptor2, false, 0, adminScope);
    setupMockParam(*mockAuthorizedUnsubscribedParam, "/authorizedUnsubscribedParam", *mockDescriptor3, false, 0, monitorScope);
    
    // Set up expectations for descriptors (all non-minimal set)
    EXPECT_CALL(*mockDescriptor1, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockDescriptor2, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockDescriptor3, minimalSet())
        .WillRepeatedly(testing::Return(false));
    
    // Set up expectations for toProto calls
    // Only authorized subscribed param should be called (minimal set param is already in fixture)
    EXPECT_CALL(*mockAuthorizedSubscribedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .WillOnce(testing::Invoke([](catena::Param& param, Authorizer& authz) {
            param.set_type(catena::ParamType::INT32);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockUnauthorizedSubscribedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called - unauthorized
    EXPECT_CALL(*mockAuthorizedUnsubscribedParam, toProto(testing::An<catena::Param&>(), testing::_))
        .Times(0); // Should not be called - unsubscribed
    
    device_->addItem("authorizedSubscribedParam", mockAuthorizedSubscribedParam.get());
    device_->addItem("unauthorizedSubscribedParam", mockUnauthorizedSubscribedParam.get());
    device_->addItem("authorizedUnsubscribedParam", mockAuthorizedUnsubscribedParam.get());
    
    // Create subscribed OIDs set with exact match
    std::set<std::string> subscribedOids = {"/authorizedSubscribedParam", "/unauthorizedSubscribedParam"};
    
    // Get device serializer with SUBSCRIPTIONS detail level and monitor authorization
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_SUBSCRIPTIONS, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundAuthorizedSubscribedParam = false;
    bool foundUnauthorizedSubscribedParam = false;
    bool foundAuthorizedUnsubscribedParam = false;
    bool foundMinimalSetParam = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // First component should be device info
        if (componentCount == 1) {
            EXPECT_TRUE(component.has_device());
            EXPECT_EQ(component.device().slot(), 1);
        }
        
        // Check for parameters
        if (component.has_param()) {
            if (component.param().oid() == "authorizedSubscribedParam") {
                foundAuthorizedSubscribedParam = true;
            } else if (component.param().oid() == "unauthorizedSubscribedParam") {
                foundUnauthorizedSubscribedParam = true;
            } else if (component.param().oid() == "authorizedUnsubscribedParam") {
                foundAuthorizedUnsubscribedParam = true;
            } else if (component.param().oid() == "minimalSetParam") {
                foundMinimalSetParam = true;
            }
        }
    }
    
    // Should have: device info (1) + authorized subscribed param (1) + minimal set param (1) = 3
    EXPECT_EQ(componentCount, 3);
    EXPECT_TRUE(foundAuthorizedSubscribedParam);    // Subscribed + authorized
    EXPECT_FALSE(foundUnauthorizedSubscribedParam); // Subscribed but unauthorized
    EXPECT_FALSE(foundAuthorizedUnsubscribedParam); // Authorized but unsubscribed
    EXPECT_TRUE(foundMinimalSetParam);             // Minimal set (always included in subscriptions mode)
}

// 6.4: Success Case - Get Device Serializer with Menus
TEST_F(DeviceTest, GetDeviceSerializer_Menus) {
    // Create mock menu groups and add them to the device
    auto mockMenuGroup = std::make_shared<MockMenuGroup>();
    auto mockMenu = std::make_unique<MockMenu>();
    
    // Set up expectations for menu toProto
    EXPECT_CALL(*mockMenu, toProto(testing::An<catena::Menu&>()))
        .WillOnce(testing::Invoke([](catena::Menu& menu) {
            auto* name = menu.mutable_name();
            name->mutable_display_strings()->insert({"en", "Test Menu"});
        }));
    
    // Set up expectations for menu group
    static std::unordered_map<std::string, std::unique_ptr<IMenu>> menuMap;
    menuMap.clear();
    menuMap["testMenu"] = std::move(mockMenu);
    EXPECT_CALL(*mockMenuGroup, menus())
        .WillRepeatedly(testing::Return(&menuMap));
    
    device_->addItem("menuGroup", mockMenuGroup.get());
    
    // Create empty subscribed OIDs set - FULL detail level should include all components
    std::set<std::string> subscribedOids = {};
    
    // Get device serializer with FULL detail level
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundMenu = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // Check for menu component
        if (component.has_menu()) {
            foundMenu = true;
            EXPECT_EQ(component.menu().oid(), "menuGroup/testMenu");
        }
    }
    
    // Should have: device info (1) + menu (1) + minimal set param (1) + language packs from SetUp (2) = 5
    EXPECT_EQ(componentCount, 5);
    EXPECT_TRUE(foundMenu);
}

// 6.5: Success Case - Get Device Serializer with Language Packs
TEST_F(DeviceTest, GetDeviceSerializer_LanguagePacks) {
    // Create empty subscribed OIDs set - language packs should be included in FULL detail level
    std::set<std::string> subscribedOids = {};
    
    // Get device serializer with FULL detail level
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundEnglish = false;
    bool foundFrench = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // Check for language pack components
        if (component.has_language_pack()) {
            if (component.language_pack().language() == "en") {
                foundEnglish = true;
            } else if (component.language_pack().language() == "fr") {
                foundFrench = true;
            }
        }
    }
    
    // Should have: device info (1) + language packs (2) + minimal set param (1) = 4
    EXPECT_EQ(componentCount, 4);
    EXPECT_TRUE(foundEnglish);
    EXPECT_TRUE(foundFrench);
}

// 6.6: Success Case - Get Device Serializer with Constraints
TEST_F(DeviceTest, GetDeviceSerializer_Constraints) {
    // Create mock constraints and add them to the device
    auto mockConstraint = std::make_shared<MockConstraint>();
    
    // Set up expectations for constraint toProto
    EXPECT_CALL(*mockConstraint, toProto(testing::An<catena::Constraint&>()))
        .WillOnce(testing::Invoke([](catena::Constraint& constraint) {
            constraint.set_ref_oid("testConstraint");
        }));
    
    device_->addItem("testConstraint", mockConstraint.get());
    
    // Create empty subscribed OIDs set - FULL detail level should include all components
    std::set<std::string> subscribedOids = {};
    
    // Get device serializer with FULL detail level
    auto serializer = device_->getDeviceSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    
    // Test that we can iterate through components
    int componentCount = 0;
    bool foundConstraint = false;
    
    while (serializer.hasMore()) {
        auto component = serializer.getNext();
        componentCount++;
        
        // Check for constraint component
        if (component.has_shared_constraint()) {
            foundConstraint = true;
            EXPECT_EQ(component.shared_constraint().oid(), "testConstraint");
        }
    }
    
    // Should have: device info (1) + constraint (1) + minimal set param (1) + language packs from SetUp (2) = 5
    EXPECT_EQ(componentCount, 5);
    EXPECT_TRUE(foundConstraint);
}

// ==== 7. Helper Function Tests ====

// 7.1: Success Case - Test Get Next method
TEST_F(DeviceTest, GetNext) {
    // Create a simple serializer to test getNext with FULL detail level
    std::set<std::string> subscribedOids = {};
    auto serializer = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    
    // Test that getNext returns components when hasMore is true
    EXPECT_TRUE(serializer->hasMore());
    auto component = serializer->getNext();
    EXPECT_TRUE(component.has_device());
    EXPECT_EQ(component.device().slot(), 1);
    
    // Test that we can get multiple components
    int componentCount = 0;
    while (serializer->hasMore()) {
        component = serializer->getNext();
        componentCount++;
    }
    
    // Should have 2 language packs from SetUp + 1 minimal set param from fixture
    // Note: The device component is returned at the end (in getDeviceSerializer)
    EXPECT_EQ(componentCount, 3);
}

// 7.2: Success Case - Test Get Component Serializer factory method
TEST_F(DeviceTest, GetComponentSerializer) {
    // Create empty subscribed OIDs set - FULL detail level should include all components
    std::set<std::string> subscribedOids = {};
    // Test shallow copy
    auto serializer0 = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, true);
    EXPECT_NE(serializer0, nullptr);
    // Test different detail levels
    auto serializer1 = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_FULL, false);
    EXPECT_NE(serializer1, nullptr);
    auto serializer2 = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_COMMANDS, false);
    EXPECT_NE(serializer2, nullptr);
    auto serializer3 = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_SUBSCRIPTIONS, false);
    EXPECT_NE(serializer3, nullptr);
    auto serializer4 = device_->getComponentSerializer(*monitorAuthz_, subscribedOids, catena::Device_DetailLevel_NONE, false);
    EXPECT_NE(serializer4, nullptr);
    // Test with empty subscriptions
    std::set<std::string> emptySubscribedOids = {};
    auto serializer5 = device_->getComponentSerializer(*monitorAuthz_, emptySubscribedOids, catena::Device_DetailLevel_FULL, false);
    EXPECT_NE(serializer5, nullptr);
}

// 7.3 - Success Case - shouldSendParam functionality
TEST_F(DeviceTest, ShouldSendParam) {
    // Create mock parameters with different characteristics
    auto mockParam = std::make_shared<MockParam>();
    auto mockCommand = std::make_shared<MockParam>();
    auto mockDescriptor = std::make_shared<MockParamDescriptor>();
    auto mockCommandDescriptor = std::make_shared<MockParamDescriptor>();
    
    // Set up authorization - use monitor scope for monitor authorization
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    setupMockParam(*mockParam, "/testParam", *mockDescriptor, false, 0, monitorScope);
    setupMockParam(*mockCommand, "/testCommand", *mockCommandDescriptor, false, 0, monitorScope);
    
    // Set up expectations for descriptors
    EXPECT_CALL(*mockDescriptor, minimalSet())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockCommandDescriptor, isCommand())
        .WillRepeatedly(testing::Return(true));
    
    // Test different detail levels with the same parameters
    device_->detail_level(catena::Device_DetailLevel_FULL); // FULL should send all
    EXPECT_TRUE(device_->shouldSendParam(*mockParam, false, *monitorAuthz_)); 
    EXPECT_TRUE(device_->shouldSendParam(*mockCommand, false, *monitorAuthz_));
    EXPECT_TRUE(device_->shouldSendParam(*mockParams_[0], false, *monitorAuthz_)); // fixture minimal set param

    device_->detail_level(catena::Device_DetailLevel_COMMANDS); // COMMANDS should send commands
    EXPECT_FALSE(device_->shouldSendParam(*mockParam, false, *monitorAuthz_)); // not a command
    EXPECT_TRUE(device_->shouldSendParam(*mockCommand, false, *monitorAuthz_)); // is a command

    device_->detail_level(catena::Device_DetailLevel_MINIMAL); // MINIMAL should send minimal
    EXPECT_FALSE(device_->shouldSendParam(*mockParam, false, *monitorAuthz_)); // not minimal
    EXPECT_TRUE(device_->shouldSendParam(*mockParams_[0], false, *monitorAuthz_)); // fixture minimal set param

    device_->detail_level(catena::Device_DetailLevel_SUBSCRIPTIONS); // SUBSCRIPTIONS should send subscribed
    EXPECT_FALSE(device_->shouldSendParam(*mockParam, false, *monitorAuthz_)); // not subscribed, not minimal
    EXPECT_TRUE(device_->shouldSendParam(*mockParam, true, *monitorAuthz_)); // subscribed
    EXPECT_TRUE(device_->shouldSendParam(*mockParams_[0], false, *monitorAuthz_)); // fixture minimal set param

    device_->detail_level(catena::Device_DetailLevel_NONE); // NONE should send nothing
    EXPECT_FALSE(device_->shouldSendParam(*mockParam, false, *monitorAuthz_));
    EXPECT_FALSE(device_->shouldSendParam(*mockCommand, false, *monitorAuthz_));
    EXPECT_FALSE(device_->shouldSendParam(*mockParams_[0], false, *monitorAuthz_)); // fixture minimal set param
}
