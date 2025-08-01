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
 * @brief This file is for testing the ParamWithValue.h file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "ParamWithValue.h"

#include "MockParamDescriptor.h"
#include "MockDevice.h"
#include "MockConstraint.h"

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class ParamWithValueTest : public ::testing::Test {
  protected:
    /*
     * 
     */
    void SetUp() override {
        EXPECT_CALL(pd_, getOid()).WillRepeatedly(testing::ReturnRef(oid_));
    }

    template <typename T>
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

    template <typename T>
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

    std::string oid_ = "test_oid";
    MockParamDescriptor pd_;
    MockDevice dm_;
};

/* ============================================================================
 *                                   EMPTY
 * ============================================================================
 * 
 * 
 */
using EmptyParam = ParamWithValue<EmptyValue>;

TEST_F(ParamWithValueTest, Empty_Create) {
    CreateTest<EmptyValue>(emptyValue);
}

TEST_F(ParamWithValueTest, Empty_Get) {
    GetValueTest<EmptyValue>(emptyValue);
}

TEST_F(ParamWithValueTest, Empty_Size) {
    EmptyParam param(emptyValue, pd_);
    EXPECT_EQ(param.size(), 0);
}

TEST_F(ParamWithValueTest, Empty_GetParam) {
    ParamWithValue<EmptyValue> param(emptyValue, pd_);
}


/* ============================================================================
 *                                  INT32_t
 * ============================================================================
 * 
 * 
 */

using IntParam = ParamWithValue<int32_t>;

TEST_F(ParamWithValueTest, Int_Create) {
    int32_t value{0};
    CreateTest<int32_t>(value);
}

TEST_F(ParamWithValueTest, Int_Get) {
    int32_t value{0};
    GetValueTest<int32_t>(value);
}

TEST_F(ParamWithValueTest, Int_Size) {
    int32_t value{0};
    IntParam param(value, pd_);
    EXPECT_EQ(param.size(), 0);
}

/* ============================================================================
 *                                   FLOAT
 * ============================================================================
 * 
 * 
 */

 using FloatParam = ParamWithValue<float>;

TEST_F(ParamWithValueTest, Float_Create) {
    float value{0};
    CreateTest<float>(value);
}

TEST_F(ParamWithValueTest, Float_Get) {
    float value{0};
    GetValueTest<float>(value);
}

TEST_F(ParamWithValueTest, Float_Size) {
    float value{0};
    FloatParam param(value, pd_);
    EXPECT_EQ(param.size(), 0);
}

/* ============================================================================
 *                                  STRING
 * ============================================================================
 * 
 * 
 */

using StringParam = ParamWithValue<std::string>;

TEST_F(ParamWithValueTest, String_Create) {
    std::string value{"Hello World"};
    CreateTest<std::string>(value);
}

TEST_F(ParamWithValueTest, String_Get) {
    std::string value{"Hello World"};
    GetValueTest<std::string>(value);
}

TEST_F(ParamWithValueTest, String_Size) {
    std::string value{"Hello World"};
    StringParam param(value, pd_);
    EXPECT_EQ(param.size(), value.size());
}


/* ============================================================================
 *                                 INT ARRAY
 * ============================================================================
 * 
 * 
 */

using IntArrayParam = ParamWithValue<std::vector<int32_t>>;

TEST_F(ParamWithValueTest, IntArray_Create) {
    std::vector<int32_t> value{0, 1, 2};
    CreateTest<std::vector<int32_t>>(value);
}

TEST_F(ParamWithValueTest, IntArray_Get) {
    std::vector<int32_t> value{0, 1, 2};
    GetValueTest<std::vector<int32_t>>(value);
}

TEST_F(ParamWithValueTest, IntArray_Size) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    EXPECT_EQ(param.size(), value.size());
}


/* ============================================================================
 *                                FLOAT ARRAY
 * ============================================================================
 * 
 * 
 */

using FloatArrayParam = ParamWithValue<std::vector<float>>;

TEST_F(ParamWithValueTest, FloatArray_Create) {
    std::vector<float> value{0, 1, 2};
    CreateTest<std::vector<float>>(value);
}

TEST_F(ParamWithValueTest, FloatArray_Get) {
    std::vector<float> value{0, 1, 2};
    GetValueTest<std::vector<float>>(value);
}

TEST_F(ParamWithValueTest, FloatArray_Size) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    EXPECT_EQ(param.size(), value.size());
}


/* ============================================================================
 *                               STRING ARRAY
 * ============================================================================
 * 
 * 
 */
using StringArrayParam = ParamWithValue<std::vector<std::string>>;

TEST_F(ParamWithValueTest, StringArray_Create) {
    std::vector<std::string> value{"Hello", "World"};
    CreateTest<std::vector<std::string>>(value);
}

TEST_F(ParamWithValueTest, StringArray_Get) {
    std::vector<std::string> value{"Hello", "World"};
    GetValueTest<std::vector<std::string>>(value);
}

TEST_F(ParamWithValueTest, StringArray_Size) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    EXPECT_EQ(param.size(), value.size());
}




/* ============================================================================
 *                                  STRUCT
 * ============================================================================
 * 
 * 
 */



/* ============================================================================
 *                               STRUCT ARRAY
 * ============================================================================
 * 
 * 
 */




/* ============================================================================
 *                              VARIANT STRUCT
 * ============================================================================
 * 
 * 
 */




/* ============================================================================
 *                           VARIANT STRUCT ARRAY
 * ============================================================================
 * 
 * 
 */




/* ============================================================================
 *                                  GENERAL
 * ============================================================================
 * 
 * 
 */

 /*
  * TEST ? - Tests a number of functions that just forward to the descriptor.
  */
TEST_F(ParamWithValueTest, DescriptorForwards) {
    EmptyParam param(emptyValue, pd_);
    // param.getDescriptor()
    EXPECT_EQ(&param.getDescriptor(), &pd_);
    // param.type()
    EXPECT_CALL(pd_, type()).Times(1).WillOnce(testing::Return(catena::ParamType::EMPTY));
    EXPECT_EQ(param.type().value(), catena::ParamType::EMPTY);
    // param.getOid()
    EXPECT_CALL(pd_, getOid()).Times(1).WillOnce(testing::ReturnRef(oid_));
    EXPECT_EQ(param.getOid(), oid_);
    // param.setOid()
    std::string newOid = "new_oid";
    EXPECT_CALL(pd_, setOid(newOid)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.setOid(newOid););
    // param.readOnly()
    EXPECT_CALL(pd_, readOnly()).Times(1).WillOnce(testing::Return(true));
    EXPECT_TRUE(param.readOnly());
    // param.readOnly(flag)
    EXPECT_CALL(pd_, readOnly(false)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.readOnly(false););
    // param.defineCommand()
    EXPECT_CALL(pd_, defineCommand(testing::_)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.defineCommand(
        [](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
            return nullptr;
        }););
    // param.executeCommand()
    catena::Value testVal;
    testVal.set_string_value("test");
    EXPECT_CALL(pd_, executeCommand(testing::_)).Times(1).WillOnce(testing::Invoke([&testVal](catena::Value value){
        EXPECT_EQ(value.string_value(), testVal.string_value());
        return nullptr;
    }));
    EXPECT_FALSE(param.executeCommand(testVal));
    // param.addParam()
    std::string subOid = "sub_oid";
    MockParamDescriptor subPd;
    EXPECT_CALL(pd_, addSubParam(subOid, &subPd)).Times(1).WillOnce(testing::Return());
    EXPECT_NO_THROW(param.addParam(subOid, &subPd););
    // param.isArrayType()
    for (auto [type, expected] : std::vector<std::pair<catena::ParamType, bool>>{
        {catena::ParamType::UNDEFINED, false},
        {catena::ParamType::EMPTY, false},
        {catena::ParamType::INT32, false},
        {catena::ParamType::FLOAT32, false},
        {catena::ParamType::STRING, false},
        {catena::ParamType::STRUCT, false},
        {catena::ParamType::STRUCT_VARIANT, false},
        {catena::ParamType::INT32_ARRAY, true},
        {catena::ParamType::FLOAT32_ARRAY, true},
        {catena::ParamType::STRING_ARRAY, true},
        {catena::ParamType::BINARY, false},
        {catena::ParamType::STRUCT_ARRAY, true},
        {catena::ParamType::STRUCT_VARIANT_ARRAY, true},
        {catena::ParamType::DATA, false}
    }) {
        MockParamDescriptor typeTestPd_;
        EmptyParam typeTestParam(emptyValue, typeTestPd_);
        EXPECT_CALL(typeTestPd_, type()).WillOnce(testing::Return(type));
        EXPECT_EQ(typeTestParam.isArrayType(), expected);
    }
    // param.getConstraint()
    MockConstraint testConstraint;
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&testConstraint));
    EXPECT_EQ(param.getConstraint(), &testConstraint);
    // param.getScope()
    std::string testScope = "test_scope";
    EXPECT_CALL(pd_, getScope()).Times(1).WillOnce(testing::ReturnRef(testScope));
    EXPECT_EQ(param.getScope(), testScope);
}

TEST_F(ParamWithValueTest, Copy) {
    int32_t value{0};
    IntParam param(value, pd_);
    std::unique_ptr<IParam> paramCopy = nullptr;
    // Copying param and checking its values.
    EXPECT_NO_THROW(paramCopy = param.copy()) << "Failed to copy ParamWithValue using copy()";
    ASSERT_TRUE(paramCopy) << "ParamWithValue copy is nullptr";
    EXPECT_EQ(&getParamValue<int32_t>(paramCopy.get()), &param.get());
    EXPECT_EQ(&paramCopy->getDescriptor(), &param.getDescriptor());
}
