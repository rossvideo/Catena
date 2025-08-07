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
 * @brief This file is a parent fixture for testing the ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/07
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include "ParamWithValue.h"
#include "StructInfo.h"
#include "Logger.h"

// mocks
#include "MockParamDescriptor.h"
#include "MockDevice.h"
#include "MockConstraint.h"
#include "MockAuthorizer.h"

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

template <typename T>
class ParamTest : public ::testing::Test {
  protected:
    /*
     * Returns the value type of the parameter we are testing with.
     */
    virtual catena::ParamType type() const = 0;

    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("ParamWithValueTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    /*
     * Sets up the mockAuthorizer and mockParamDescriptors with default
     * behaviour.
     */
    void SetUp() override {
        // Forwards calls to authz(Param) to authz(ParamDescriptor), which is
        // not too far from it's actual implementation.
        EXPECT_CALL(authz_, readAuthz(testing::An<const IParam&>()))
            .WillRepeatedly(testing::Invoke([this](const IParam& p){
                return authz_.readAuthz(p.getDescriptor());
            }));
        EXPECT_CALL(authz_, writeAuthz(testing::An<const IParam&>()))
            .WillRepeatedly(testing::Invoke([this](const IParam& p){
                return authz_.writeAuthz(p.getDescriptor());
            }));
        for (auto* pd : {&pd_, &subpd1_, &subpd2_}) {
            // Authorizer has read and write authz for all params by default.
            EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
            EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
            // Some default values from paramDescriptor.
            EXPECT_CALL(*pd, getOid()).WillRepeatedly(testing::ReturnRef(oid_));
            EXPECT_CALL(*pd, getConstraint()).WillRepeatedly(testing::Return(nullptr));
            EXPECT_CALL(*pd, max_length()).WillRepeatedly(testing::Return(1000));
            EXPECT_CALL(*pd, total_length()).WillRepeatedly(testing::Return(1000));
            EXPECT_CALL(*pd, type()).WillRepeatedly(testing::Return(type()));
            EXPECT_CALL(*pd, toProto(testing::An<catena::Param&>(), testing::_))
                .WillRepeatedly(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
                    p.set_template_oid(oid_);
                }));
            // Default paramDescriptor tree:
            //  Any oid ending with 1 routes to subpd1_.
            //  Any oid ending with 2 routes to subpd2_.
            EXPECT_CALL(*pd, getSubParam("f1")).WillRepeatedly(testing::ReturnRef(subpd1_));
            EXPECT_CALL(*pd, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(subpd1_));
            EXPECT_CALL(*pd, getSubParam("f2")).WillRepeatedly(testing::ReturnRef(subpd2_));
            EXPECT_CALL(*pd, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(subpd2_));
        }
    }

    /*
     * Function which tests 3/4 of ParamWithValue's constructors.
     * The last constructor is CatenaStruct specific.
     */
    void CreateTest(T& value) {
        // Constructor (value, descriptor, device, isCommand)
        EXPECT_CALL(dm_, addItem(oid_, testing::An<IParam*>())).Times(1)
            .WillOnce(testing::Invoke([](std::string, IParam* param) {
                EXPECT_TRUE(param) << "Nullptr added to dm_";
            }));
        EXPECT_NO_THROW(ParamWithValue<T>(value, pd_, dm_, false);)
            << "Failed to create a ParamWithValue using constructor"
            << "(value, descriptor, device, isCommand)";
        // Constructor (value, descriptor)
        EXPECT_NO_THROW(ParamWithValue<T>(value, pd_);)
            << "Failed to create a ParamWithValue using constructor"
            << "(value, descriptor)";
        // Constructor (value, descriptor, mSizeTracker, tSizeTracker)
        std::shared_ptr<std::size_t> mSizeTracker{0};
        std::shared_ptr<ParamWithValue<EmptyValue>::TSizeTracker> tSizeTracker{};
        EXPECT_NO_THROW(ParamWithValue<T>(value, pd_, mSizeTracker, tSizeTracker);)
            << "Failed to create a ParamWithValue using constructor"
            << "(value, descriptor, mSizeTracker, tSizeTracker)";
    }
    /*
     * Functions which tests all 3 of ParamWithValue's value getters.
     */
    void GetValueTest(T& value) {
        ParamWithValue<T> param(value, pd_);
        // Non-const
        EXPECT_EQ(&param.get(), &value);
        // Const
        const ParamWithValue<T> constParam(value, pd_);
        EXPECT_EQ(&constParam.get(), &value);
        // getParamValue
        EXPECT_EQ(&getParamValue<T>(&param), &value);
    }

    MockParamDescriptor pd_, subpd1_, subpd2_;
    MockDevice dm_;
    MockAuthorizer authz_;

    catena::exception_with_status rc_{"", catena::StatusCode::OK};
    std::string oid_ = "test_oid";
};
