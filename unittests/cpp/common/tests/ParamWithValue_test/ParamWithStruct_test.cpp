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
 * @brief This file is for testing the <CatenaStruct>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"
#include "CommonTestHelpers.h"

using namespace catena::common;

using StructParam = ParamWithValue<TestStruct1>;
using NestedStructParam = ParamWithValue<TestNestedStruct>;

class ParamWithStructTest : public ParamTest<TestStruct1> {
  protected:
    catena::ParamType type() const override { return catena::ParamType::STRUCT; }

    TestStruct1 value_{16, 32};
};

/**
 * TEST 1 - Testing <CatenaStruct>ParamWithValue constructors
 */
TEST_F(ParamWithStructTest, Create) {
    CreateTest(value_);
    // Additional constructor for creating struct field from fieldInfo.
    EXPECT_CALL(pd_, getSubParam(std::get<0>(StructInfo<TestStruct1>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd1_));
    using FieldType = typename std::tuple_element_t<0, StructInfo<TestStruct1>::Type>::Field;
    ParamWithValue<FieldType> param{std::get<0>(StructInfo<TestStruct1>::fields), value_, pd_};
    // Make sure value and descriptor are set correctly
    EXPECT_EQ(param.get(), value_.f1);
    EXPECT_EQ(&param.getDescriptor(), &subpd1_);
}

/**
 * TEST 2 - Testing <CatenaStruct>ParamWithValue.get()
 */
TEST_F(ParamWithStructTest, Get) {
    GetValueTest(value_);
}

TEST_F(ParamWithStructTest, Size) {
    StructParam param(value_, pd_);
    EXPECT_EQ(param.size(), 0);
}

TEST_F(ParamWithStructTest, GetParam) {
    StructParam param(value_, pd_);
    Path path = Path("/f1");
    EXPECT_CALL(pd_, getSubParam(std::get<0>(StructInfo<TestStruct1>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd1_));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), value_.f1);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd1_);
}

TEST_F(ParamWithStructTest, GetParam_Nested) {
    TestNestedStruct nestedValue{value_, {1.1, 2.2}};
    NestedStructParam param(nestedValue, pd_);
    Path path = Path("/f1/f1");
    EXPECT_CALL(pd_, getSubParam(std::get<0>(StructInfo<TestNestedStruct>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd1_));
    EXPECT_CALL(subpd1_, getSubParam(std::get<0>(StructInfo<TestStruct1>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd2_));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), value_.f1);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd2_);
}

/**
 * TEST 5 - Testing <CatenaStruct>ParamWithValue.getParam() error handling.
 * 
 * 4 main error cases:
 *  - Front is not a string.
 *  - Field does not exist.
 *  - Not authorized for main param.
 *  - Not authorized for sub param.
 */
TEST_F(ParamWithStructTest, GetParam_Error) {
    StructParam param(value_, pd_);
    { // Front is not a string.
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not a string";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Field does not exist
    Path path = Path("/f3");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if field does not exist";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for main param.
    Path path = Path("/f1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for main param";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for sub param.
    Path path = Path("/f1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(true));
    EXPECT_CALL(pd_, getSubParam(std::get<0>(StructInfo<TestNestedStruct>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd1_));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd1_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for sub param";
    }
}
