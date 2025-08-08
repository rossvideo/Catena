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
 * @brief This file is for testing the <STRUCT>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/08
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"
#include "CommonTestHelpers.h"

using namespace catena::common;
using StructParam = ParamWithValue<TestStruct1>;
using NestedStructParam = ParamWithValue<TestNestedStruct>;

// Fixture
class ParamWithStructTest : public ParamTest<TestStruct1> {
  protected:
    /*
     * Returns the value type of the parameter we are testing with.
     */
    catena::ParamType type() const override { return catena::ParamType::STRUCT; }

    TestStruct1 value_{16, 32};
};

/*
 * TEST 1 - Testing <STRUCT>ParamWithValue constructors.
 */
TEST_F(ParamWithStructTest, Create) {
    CreateTest(value_);
    // Additional constructor for creating struct field from fieldInfo.
    using FieldType = typename std::tuple_element_t<0, StructInfo<TestStruct1>::Type>::Field;
    ParamWithValue<FieldType> param{std::get<0>(StructInfo<TestStruct1>::fields), value_, pd_};
    // Make sure value and descriptor are set correctly
    EXPECT_EQ(param.get(), value_.f1);
    EXPECT_EQ(&param.getDescriptor(), &subpd1_);
}
/*
 * TEST 2 - Testing <STRUCT>ParamWithValue.get().
 */
TEST_F(ParamWithStructTest, Get) {
    GetValueTest(value_);
}
/*
 * TEST 3 - Testing <STRUCT>ParamWithValue.size().
 */
TEST_F(ParamWithStructTest, Size) {
    StructParam param(value_, pd_);
    EXPECT_EQ(param.size(), 0);
}
/*
 * TEST 4 - Testing <STRUCT>ParamWithValue.getParam().
 */
TEST_F(ParamWithStructTest, GetParam) {
    StructParam param(value_, pd_);
    Path path = Path("/f1");
    auto foundParam = param.getParam(path, authz_, rc_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), value_.f1);
    EXPECT_NE(&foundParam->getDescriptor(), &pd_) << "Subparam should have its own param descriptor.";
}
/*
 * TEST 5 - Testing <STRUCT>ParamWithValue.getParam() for a nested parameter.
 */
TEST_F(ParamWithStructTest, GetParam_Nested) {
    TestNestedStruct nestedValue{value_, {1.1, 2.2}};
    NestedStructParam param(nestedValue, pd_);
    Path path = Path("/f1/f1");
    EXPECT_CALL(subpd1_, getSubParam(std::get<0>(StructInfo<TestStruct1>::fields).name))
        .Times(1).WillOnce(testing::ReturnRef(subpd2_));
    auto foundParam = param.getParam(path, authz_, rc_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), value_.f1);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd2_) << "Subparam should have its own param descriptor.";
}
/*
 * TEST 6 - Testing <STRUCT>ParamWithValue.getParam() error handling.
 * Four main error cases:
 *  - Front of path is not a field name (string).
 *  - Specified field does not exist.
 *  - Not authorized for the main param.
 *  - Not authorized for the sub param.
 */
TEST_F(ParamWithStructTest, GetParam_Error) {
    StructParam param(value_, pd_);
    { // Front of path is not a field name (string).
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not a string.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Specified field does not exist.
    Path path = Path("/f3");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if field does not exist.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for the main param.
    Path path = Path("/f1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for the main param.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for the sub param.
    Path path = Path("/f1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(true));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd1_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for the sub param.";
    }
}
/*
 * TEST 7 - Testing <STRUCT>ParamWithValue.addBack().
 */
TEST_F(ParamWithStructTest, AddBack) {
    StructParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 8 - Testing <STRUCT>ParamWithValue.popBack().
 */
TEST_F(ParamWithStructTest, PopBack) {
    StructParam param(value_, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 9 - Testing <STRUCT>ParamWithValue.toProto().
 */
TEST_F(ParamWithStructTest, ParamToProto) {
    StructParam param(value_, pd_);
    catena::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    // Checking results.
    ASSERT_TRUE(outParam.value().has_struct_value());
    TestStruct1 outValue{0,0};
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK)
        << "fromProto failed, cannot continue test.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value_.f1, outValue.f1);
    EXPECT_EQ(value_.f2, outValue.f2);
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 10 - Testing <STRUCT>ParamWithValue.fromProto().
 */
TEST_F(ParamWithStructTest, FromProto) {
    TestStruct1 emptyVal{0, 0};
    StructParam param(emptyVal, pd_);
    catena::Value protoValue;
    ASSERT_EQ(toProto(protoValue, &value_, pd_, authz_).status, catena::StatusCode::OK)
        << "toProto failed, cannot continue test.";
    rc_ = param.fromProto(protoValue, authz_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get().f1, value_.f1);
    EXPECT_EQ(param.get().f2, value_.f2);
}
/*
 * TEST 11 - Testing <STRUCT>ParamWithValue.ValidateSetValue().
 */
TEST_F(ParamWithStructTest, ValidateSetValue) {
    StructParam param(value_, pd_);
    catena::Value protoValue, f1, f2;
    f1.set_int32_value(48);
    f2.set_int32_value(64);
    auto fields = protoValue.mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
}
/*
 * TEST 10 - Testing <STRUCT>ParamWithValue.ValidateSetValue() error handling.
 * Two main error cases:
 *  - Index is defined.
 *  - validFromProto returns false.
 */
TEST_F(ParamWithStructTest, ValidateSetValue_Error) {
    StructParam param(value_, pd_);
    TestStruct1 newValue{48, 64};
    catena::Value protoValue;
    ASSERT_EQ(toProto(protoValue, &newValue, pd_, authz_).status, catena::StatusCode::OK)
        << "toProto failed, cannot continue test.";
    // Defined index w non-array
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for typeA -> typeA SetValue.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for typeA -> typeA SetValue.";
    // ValidFromProto error (no authz)
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false.";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "In this case validFromProto should fail from incorrect authz.";
}
