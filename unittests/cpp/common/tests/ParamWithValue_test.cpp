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
#include "StructInfo.h"

#include "MockParamDescriptor.h"
#include "MockDevice.h"
#include "MockConstraint.h"
#include "MockAuthorizer.h"

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
        // Forwards calls to authz(Param) to authz(ParamDescriptor)
        EXPECT_CALL(authz_, readAuthz(testing::An<const IParam&>()))
            .WillRepeatedly(testing::Invoke([this](const IParam& p){
                return authz_.readAuthz(p.getDescriptor());
            }));
        EXPECT_CALL(authz_, writeAuthz(testing::An<const IParam&>()))
            .WillRepeatedly(testing::Invoke([this](const IParam& p){
                return authz_.writeAuthz(p.getDescriptor());
            }));
        // Authorizer has read and write authz by default
        EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillRepeatedly(testing::Return(true));
        EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillRepeatedly(testing::Return(true));
        // Some default values from paramDescriptor.
        EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(nullptr));
        EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(1000));
        EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(1000));
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
    MockAuthorizer authz_;
    catena::exception_with_status rc_{"", catena::StatusCode::OK};
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
    EmptyParam param(emptyValue, pd_);
    Path path = Path("/test/oid");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillRepeatedly(testing::Return(true));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Empty_AddBack) {
    EmptyParam param(emptyValue, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Empty_PopBack) {
    EmptyParam param(emptyValue, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Empty_ParamToProto) {
    EmptyParam param(emptyValue, pd_);
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, Empty_FromProto) {
    EmptyParam param(emptyValue, pd_);
    catena::Value protoValue;
    protoValue.empty_value();
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(&param.get(), &emptyValue);
}

TEST_F(ParamWithValueTest, Empty_ValidateSetValue) {
    EmptyParam param(emptyValue, pd_);
    catena::Value protoValue;
    protoValue.empty_value();
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
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

TEST_F(ParamWithValueTest, Int_GetParam) {
    int32_t value{0};
    IntParam param(value, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Int_AddBack) {
    int32_t value{0};
    IntParam param(value, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Int_PopBack) {
    int32_t value{0};
    IntParam param(value, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Int_ParamToProto) {
    int32_t value{16};
    IntParam param(value, pd_);
    int32_t outValue;
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_int32_value());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, Int_FromProto) {
    int32_t value{0};
    IntParam param(value, pd_);
    int32_t newValue{16};
    catena::Value protoValue;
    protoValue.set_int32_value(newValue);
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), newValue);
}

TEST_F(ParamWithValueTest, Int_ValidateSetValue) {
    int32_t value{0};
    IntParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_int32_value(16);
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, Int_ValidateSetValueError) {
    int32_t value{0};
    IntParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_int32_value(16);
    { // Defined index w non-array
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for non-array param";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for non-array param";
    }
    { // ValidFromProto error
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "In this case validFromProto should return PERMISSION_DENIED";
    }
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

TEST_F(ParamWithValueTest, Float_GetParam) {
    float value{0};
    FloatParam param(value, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Float_AddBack) {
    float value{0};
    FloatParam param(value, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Float_PopBack) {
    float value{0};
    FloatParam param(value, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, Float_ParamToProto) {
    float value{16};
    FloatParam param(value, pd_);
    float outValue;
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_float32_value());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, Float_FromProto) {
    float value{0};
    FloatParam param(value, pd_);
    float newValue{16};
    catena::Value protoValue;
    protoValue.set_float32_value(newValue);
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), newValue);
}

TEST_F(ParamWithValueTest, Float_ValidateSetValue) {
    float value{0};
    FloatParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(16);
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, Float_ValidateSetValueError) {
    float value{0};
    FloatParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(16);
    { // Defined index w non-array
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for non-array param";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for non-array param";
    }
    { // ValidFromProto error
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "In this case validFromProto should return PERMISSION_DENIED";
    }
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

TEST_F(ParamWithValueTest, String_GetParam) {
    std::string value{"Hello World"};
    StringParam param(value, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, String_AddBack) {
    std::string value{"Hello World"};
    StringParam param(value, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, String_PopBack) {
    std::string value{"Hello World"};
    StringParam param(value, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithValueTest, String_ParamToProto) {
    std::string value{"Hello World"};
    StringParam param(value, pd_);
    std::string outValue;
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_string_value());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, String_FromProto) {
    std::string value{""};
    StringParam param(value, pd_);
    std::string newValue{"Hello World"};
    catena::Value protoValue;
    protoValue.set_string_value(newValue);
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), newValue);
}

TEST_F(ParamWithValueTest, String_ValidateSetValue) {
    std::string value{""};
    StringParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Hello World");
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, String_ValidateSetValueError) {
    std::string value{""};
    StringParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Hello World");
    { // Defined index w non-array
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for non-array param";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for non-array param";
    }
    { // New value exceeds maxLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(5));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false valueFromProto returns false from string value exceeding max_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "ValidateSetValue should return OUT_OF_RANGE when the new string value exceeds max_length";
    }
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

TEST_F(ParamWithValueTest, IntArray_GetParam) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), value[0]);
}

TEST_F(ParamWithValueTest, IntArray_GetParamError) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    { // Front is not an index.
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Index out of bounds
    Path path = Path("/" + std::to_string(value.size()));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "getParam should return OUT_OF_RANGE if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Param does not exist
    Path path = Path("/0/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if attempting to retrieve a sub-parameter that does not exist";
    }
    { // Not authorized.
    Path path = Path("/0");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz";
    }
}

TEST_F(ParamWithValueTest, IntArray_AddBack) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, IntArray_AddBackError) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    { // Add exceeds max length
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(3));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter at max length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "addBack should return OUT_OF_RANGE if array is at max length";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter without write authz";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "addBack should return PERMISSION_DENIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, IntArray_PopBack) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    rc_ = param.popBack(authz_);
    value.pop_back();
    EXPECT_EQ(param.get(), value);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, IntArray_PopBackError) {
    std::vector<int32_t> value{};
    IntArrayParam param(value, pd_);
    { // Empty array
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "popBack should return OUT_OF_RANGE if array empty";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "popBack should return PERMISSION_DENNIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, IntArray_ParamToProto) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    std::vector<int32_t> outValue{};
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_int32_array_values());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, IntArray_ValidateSetValue) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    catena::Value protoValue;
    for (uint32_t i : {0, 1, 2}) {
        protoValue.mutable_int32_array_values()->add_ints(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

TEST_F(ParamWithValueTest, IntArray_ValidateSetValueSingleElement) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_int32_value(3);
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

TEST_F(ParamWithValueTest, IntArray_ValidateSetValueError) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    catena::Value protoValue;
    for (uint32_t i : {0, 1, 2, 3}) {
        protoValue.mutable_int32_array_values()->add_ints(i);
    }

    // Defined index with non-single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "Should return false when index is defined for non-element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when index is defined for non-element setValue";

    // New value exceeds maxLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds maxLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds maxLength";
}

TEST_F(ParamWithValueTest, IntArray_ValidateSetValueSingleElementError) {
    std::vector<int32_t> value{0, 1, 2};
    IntArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_int32_value(3);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));

    // Undefined index with single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the index is undefined for single element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when the index is undefined for single element setValue";

    // Defined index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value.size(), authz_, rc_))
        << "Should return false when the index is out of bounds of the array";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the index is out of bounds of the array";

    // Too many appends
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 2 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 2 elements";
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Should return false when the array length exceeds max_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds max_length";
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

TEST_F(ParamWithValueTest, FloatArray_GetParam) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<float>(foundParam.get()), value[0]);
}

TEST_F(ParamWithValueTest, FloatArray_GetParamError) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    { // Front is not an index.
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Index out of bounds
    Path path = Path("/" + std::to_string(value.size()));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "getParam should return OUT_OF_RANGE if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Param does not exist
    Path path = Path("/0/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if attempting to retrieve a sub-parameter that does not exist";
    }
    { // Not authorized.
    Path path = Path("/0");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz";
    }
}

TEST_F(ParamWithValueTest, FloatArray_AddBack) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, FloatArray_AddBackError) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    { // Add exceeds max length
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(3));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter at max length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "addBack should return OUT_OF_RANGE if array is at max length";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter without write authz";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "addBack should return PERMISSION_DENIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, FloatArray_PopBack) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    rc_ = param.popBack(authz_);
    value.pop_back();
    EXPECT_EQ(param.get(), value);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, FloatArray_PopBackError) {
    std::vector<float> value{};
    FloatArrayParam param(value, pd_);
    { // Empty array
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "popBack should return OUT_OF_RANGE if array empty";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "popBack should return PERMISSION_DENNIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, FloatArray_ParamToProto) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    catena::Param outParam;
    std::vector<float> outValue{};
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_float32_array_values());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, FloatArray_ValidateSetValue) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    catena::Value protoValue;
    for (float i : {0, 1, 2}) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

TEST_F(ParamWithValueTest, FloatArray_ValidateSetValueSingleElement) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(3);
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

TEST_F(ParamWithValueTest, FloatArray_ValidateSetValueError) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    catena::Value protoValue;
    for (float i : {0, 1, 2, 3}) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }

    // Defined index with non-single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "Should return false when index is defined for non-element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when index is defined for non-element setValue";

    // New value exceeds maxLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds maxLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds maxLength";
}

TEST_F(ParamWithValueTest, FloatArray_ValidateSetValueSingleElementError) {
    std::vector<float> value{0, 1, 2};
    FloatArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(3);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));

    // Undefined index with single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the index is undefined for single element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when the index is undefined for single element setValue";

    // Defined index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value.size(), authz_, rc_))
        << "Should return false when the index is out of bounds of the array";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the index is out of bounds of the array";

    // Too many appends
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 2 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 2 elements";
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Should return false when the array length exceeds max_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds max_length";
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

TEST_F(ParamWithValueTest, StringArray_GetParam) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<std::string>(foundParam.get()), value[0]);
}

TEST_F(ParamWithValueTest, StringArray_GetParamError) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    { // Front is not an index.
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Index out of bounds
    Path path = Path("/" + std::to_string(value.size()));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "getParam should return OUT_OF_RANGE if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Param does not exist
    Path path = Path("/0/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if attempting to retrieve a sub-parameter that does not exist";
    }
    { // Not authorized.
    Path path = Path("/0");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz";
    }
}

TEST_F(ParamWithValueTest, StringArray_AddBack) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, StringArray_AddBackError) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    { // Add exceeds max length
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(2));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter at max length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "addBack should return OUT_OF_RANGE if array is at max length";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter without write authz";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "addBack should return PERMISSION_DENIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, StringArray_PopBack) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    rc_ = param.popBack(authz_);
    value.pop_back();
    EXPECT_EQ(param.get(), value);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithValueTest, StringArray_PopBackError) {
    std::vector<std::string> value{};
    StringArrayParam param(value, pd_);
    { // Empty array
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "popBack should return OUT_OF_RANGE if array empty";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "popBack should return PERMISSION_DENNIED if Authorizer does not have writeAuthz";
    }
}

TEST_F(ParamWithValueTest, StringArray_ParamToProto) {
    std::vector<std::string> value{};
    StringArrayParam param(value, pd_);
    std::vector<std::string> outValue{};
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_string_array_values());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithValueTest, StringArray_ValidateSetValue) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    for (std::string i : {"Hello", "World", "!"}) {
        protoValue.mutable_string_array_values()->add_strings(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

TEST_F(ParamWithValueTest, StringArray_ValidateSetValueSingleElement) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Goodbye");
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

TEST_F(ParamWithValueTest, StringArray_ValidateSetValueError) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    for (std::string i : {"Hello", "World", "Goodbye"}) {
        protoValue.mutable_string_array_values()->add_strings(i);
    }

    // Defined index with non-single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "Should return false when index is defined for non-element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when index is defined for non-element setValue";

    // New value exceeds maxLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds maxLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds maxLength";
    
    // New value exceeds totalLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(1000));
    EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(10));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds totalLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds totalLength";
}

TEST_F(ParamWithValueTest, StringArray_ValidateSetValueSingleElementError) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("!");
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));

    // Undefined index with single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the index is undefined for single element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when the index is undefined for single element setValue";

    // Defined index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value.size(), authz_, rc_))
        << "Should return false when the index is out of bounds of the array";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the index is out of bounds of the array";

    // Too many appends
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Should return false when the array length exceeds max_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds max_length";
    
    // Exceeds total_length
    param.resetValidate();
    protoValue.set_string_value("This is a long string");
    EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(15));
    EXPECT_FALSE(param.validateSetValue(protoValue, 0, authz_, rc_))
        << "Should return false when the array length exceeds total_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds total_length";
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

TEST_F(ParamWithValueTest, ParamToProtoError) {
    int32_t value{16};
    IntParam param(value, pd_);
    catena::Param outParam;
    { // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
    EXPECT_THROW(param.toProto(outParam, authz_), std::runtime_error);
    }
    { // No read authz
    outParam.Clear();
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.toProto(outParam, authz_);
    EXPECT_FALSE(outParam.value().has_int32_value());
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED);
    }
}

TEST_F(ParamWithValueTest, ParamInfoToProto) {
    EmptyParam param(emptyValue, pd_);
    catena::ParamInfoResponse paramInfo;
    EXPECT_CALL(pd_, toProto(testing::An<catena::ParamInfo&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::ParamInfo& p, const IAuthorizer&) {
            p.set_oid(oid_);
        }));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, paramInfo.info().oid());
}

TEST_F(ParamWithValueTest, ParamInfoToProtoError) {
    EmptyParam param(emptyValue, pd_);
    catena::ParamInfoResponse paramInfo;
    { // pd_.toProto throws an error
    EXPECT_CALL(pd_, toProto(testing::An<catena::ParamInfo&>(), testing::_)).WillOnce(testing::Throw(std::runtime_error("Test error")));
    EXPECT_THROW(param.toProto(paramInfo, authz_), std::runtime_error);
    }
    { // No read authz
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.toProto(paramInfo, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED);
    }
}
