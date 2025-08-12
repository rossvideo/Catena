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
 * @brief This file is for testing the <VARIANT>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/08
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"
#include "CommonTestHelpers.h"

using namespace catena::common;
using VariantParam = ParamWithValue<TestVariantStruct>;

// Fixture
class ParamWithVariantTest : public ParamTest<TestVariantStruct> {
  protected:
    /*
     * Returns the value type of the parameter we are testing with.
     */
    st2138::ParamType type() const override { return st2138::ParamType::STRUCT_VARIANT; }

    TestVariantStruct value_{TestStruct1{.f1{16}, .f2{32}}};
};

/*
 * TEST 1 - Testing <VARIANT>ParamWithValue constructors.
 */
TEST_F(ParamWithVariantTest, Create) {
    CreateTest(value_);
}
/*
 * TEST 2 - Testing <VARIANT>ParamWithValue.get().
 */
TEST_F(ParamWithVariantTest, Get) {
    GetValueTest(value_);
}
/*
 * TEST 3 - Testing <VARIANT>ParamWithValue.size().
 */
TEST_F(ParamWithVariantTest, Size) {
    VariantParam param(value_, pd_);
    EXPECT_EQ(param.size(), 0);
}
/*
 * TEST 4 - Testing <VARIANT>ParamWithValue.getParam().
 */
TEST_F(ParamWithVariantTest, GetParam) {
    VariantParam param(value_, pd_);
    { // Get variant struct
    Path path = Path("/TestStruct1");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f1, std::get<TestStruct1>(value_).f1);
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f2, std::get<TestStruct1>(value_).f2);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd1_)
        << "Variant struct should have a unique param descriptor depending on its actual type.";
    }
    rc_ = catena::exception_with_status("", catena::StatusCode::OK); // Reset status
    { // Get variant struct field
    Path path = Path("/TestStruct1/f2");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<int32_t>(foundParam.get()), std::get<TestStruct1>(value_).f2);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd2_) << "Subparam should have its own param descriptor.";
    }
}
/*
 * TEST 5 - Testing <VARIANT>ParamWithValue.getParam() error handling.
 * Four main error cases:
 *  - Front of path is not a struct type (string).
 *  - Struct type does not exist.
 *  - Not authorized for the main param.
 *  - Not authorized for the sub param.
 */
TEST_F(ParamWithVariantTest, GetParam_Error) {
    VariantParam param(value_, pd_);
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(subpd1_));
    { // Front of path is not a string.
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not a string.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Struct type does not exist
    Path path = Path("/nonExistentStruct");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if field does not exist.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for main param.
    Path path = Path("/TestStruct1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for the main param.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized for struct type.
    Path path = Path("/TestStruct1");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(true));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd1_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz for the sub param.";
    }
}
/*
 * TEST 6 - Testing <VARIANT>ParamWithValue.addBack().
 */
TEST_F(ParamWithVariantTest, AddBack) {
    VariantParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 7 - Testing <VARIANT>ParamWithValue.popBack().
 */
TEST_F(ParamWithVariantTest, PopBack) {
    VariantParam param(value_, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 8 - Testing <VARIANT>ParamWithValue.toProto().
 */
TEST_F(ParamWithVariantTest, ParamToProto) {
    VariantParam param(value_, pd_);
    st2138::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    // Checking results.
    ASSERT_TRUE(outParam.value().has_struct_variant_value());
    EXPECT_EQ(outParam.value().struct_variant_value().struct_variant_type(), "TestStruct1");
    TestVariantStruct outValue{TestStruct1{0,0}};
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK)
        << "fromProto failed, cannot continue test.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(std::get<TestStruct1>(value_).f1, std::get<TestStruct1>(outValue).f1);
    EXPECT_EQ(std::get<TestStruct1>(value_).f2, std::get<TestStruct1>(outValue).f2);
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 9 - Testing <VARIANT>ParamWithValue.fromProto().
 */
TEST_F(ParamWithVariantTest, FromProto) {
    TestVariantStruct emptyVal{TestStruct2{0, 0}};
    VariantParam param(emptyVal, pd_);
    st2138::Value protoValue;
    ASSERT_EQ(toProto(protoValue, &value_, pd_, authz_).status, catena::StatusCode::OK)
        << "toProto failed, cannot continue test.";
    rc_ = param.fromProto(protoValue, authz_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(alternativeNames<TestVariantStruct>[param.get().index()], protoValue.struct_variant_value().struct_variant_type());
    EXPECT_EQ(std::get<TestStruct1>(param.get()).f1, std::get<TestStruct1>(value_).f1);
    EXPECT_EQ(std::get<TestStruct1>(param.get()).f2, std::get<TestStruct1>(value_).f2);
}
/*
 * TEST 10 - Testing <VARIANT>ParamWithValue.ValidateSetValue().
 */
TEST_F(ParamWithVariantTest, ValidateSetValue) {
    VariantParam param(value_, pd_);
    TestVariantStruct newValue{TestStruct1{48, 64}};
    st2138::Value protoValue;
    ASSERT_EQ(toProto(protoValue, &newValue, pd_, authz_).status, catena::StatusCode::OK)
        << "toProto failed, cannot continue test.";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
}
/*
 * TEST 11 - Testing <VARIANT>ParamWithValue.ValidateSetValue() error handling.
 * Two main error cases:
 *  - Index is defined.
 *  - validFromProto returns false.
 */
TEST_F(ParamWithVariantTest, ValidateSetValue_Error) {
    VariantParam param(value_, pd_);
    TestVariantStruct newValue{TestStruct1{48, 64}};
    st2138::Value protoValue;
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
