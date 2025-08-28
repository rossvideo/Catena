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
 * @brief This file is for testing the <FLOAT_ARRAY>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/07
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;
using FloatArray = std::vector<float>;
using FloatArrayParam = ParamWithValue<FloatArray>;

// Fixture
class ParamWithFloatArrayTest : public ParamTest<FloatArray> {
  protected:
    /*
     * Returns the value type of the parameter we are testing with.
     */
    st2138::ParamType type() const override { return st2138::ParamType::FLOAT32_ARRAY; }

    FloatArray value_{0, 1, 2};
};

/*
 * TEST 1 - Testing <FLOAT_ARRAY>ParamWithValue constructors.
 */
TEST_F(ParamWithFloatArrayTest, Create) {
    CreateTest(value_);
}
/*
 * TEST 2 - Testing <FLOAT_ARRAY>ParamWithValue.get().
 */
TEST_F(ParamWithFloatArrayTest, Get) {
    GetValueTest(value_);
}
/*
 * TEST 3 - Testing <FLOAT_ARRAY>ParamWithValue.size().
 */
TEST_F(ParamWithFloatArrayTest, Size) {
    FloatArrayParam param(value_, pd_);
    EXPECT_EQ(param.size(), value_.size());
}
/*
 * TEST 4 - Testing <FLOAT_ARRAY>ParamWithValue.getParam().
 * FLOAT_ARRAY params can use getParam to access individual elements.
 */
TEST_F(ParamWithFloatArrayTest, GetParam) {
    FloatArrayParam param(value_, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<float>(foundParam.get()), value_[0]);
    EXPECT_EQ(&foundParam->getDescriptor(), &pd_) << "Element should inherit the parent descriptor.";
}
/*
 * TEST 5 - Testing <FLOAT_ARRAY>ParamWithValue.getParam() error handling.
 * Four main error cases:
 *  - Front of path is not an index.
 *  - Index is out of bounds.
 *  - Attempting to retrieve an element's non-existent sub-parameter.
 *  - Not authorized.
 */
TEST_F(ParamWithFloatArrayTest, GetParam_Error) {
    FloatArrayParam param(value_, pd_);
    { // Front of path is not an index.
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not an index.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Index is out of bounds
    Path path = Path("/" + std::to_string(value_.size()));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "getParam should return OUT_OF_RANGE if the index is out of bounds.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Attempting to retrieve an element's non-existent sub-parameter.
    Path path = Path("/0/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::NOT_FOUND)
        << "getParam should return NOT_FOUND if attempting to retrieve an element's non-existent sub-parameter.";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized.
    Path path = Path("/0");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected.";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz.";
    }
}
/*
 * TEST 6 - Testing <FLOAT_ARRAY>ParamWithValue.addBack().
 */
TEST_F(ParamWithFloatArrayTest, AddBack) {
    FloatArrayParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/*
 * TEST 7 - Testing <FLOAT_ARRAY>ParamWithValue.addBack() error handling.
 * Two main error cases:
 *  - Adding a value exceeds max length.
 *  - Not authorized.
 */
TEST_F(ParamWithFloatArrayTest, AddBack_Error) {
    FloatArrayParam param(value_, pd_);
    { // Add exceeds max length
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(value_.size()));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter at max length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "addBack should return OUT_OF_RANGE if array is at max length";
    }
    { // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to array parameter without writeAuthz";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "addBack should return PERMISSION_DENIED if Authorizer does not have writeAuthz";
    }
}
/*
 * TEST 8 - Testing <FLOAT_ARRAY>ParamWithValue.popBack().
 */
TEST_F(ParamWithFloatArrayTest, PopBack) {
    FloatArrayParam param(value_, pd_);
    FloatArray valueCopy{value_.begin(), value_.end()};
    rc_ = param.popBack(authz_);
    valueCopy.pop_back();
    EXPECT_EQ(param.get(), valueCopy);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/*
 * TEST 9 - Testing <FLOAT_ARRAY>ParamWithValue.popBack() error handling.
 * Two main error cases:
 * - Array is empty.
 * - Not authorized.
 */
TEST_F(ParamWithFloatArrayTest, PopBack_Error) {
    FloatArray value{};
    FloatArrayParam param(value, pd_);
    // Empty array
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "popBack should return OUT_OF_RANGE if array empty";
    // Not authorized
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "popBack should return PERMISSION_DENNIED if Authorizer does not have writeAuthz";
}
/*
 * TEST 10 - Testing <FLOAT_ARRAY>ParamWithValue.toProto().
 */
TEST_F(ParamWithFloatArrayTest, ParamToProto) {
    FloatArrayParam param(value_, pd_);
    st2138::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    // Checking results.
    ASSERT_TRUE(outParam.value().has_float32_array_values());
    FloatArray outValue{};
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK)
        << "fromProto failed, cannot continue test.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value_, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 11 - Testing <FLOAT_ARRAY>ParamWithValue.fromProto().
 */
TEST_F(ParamWithFloatArrayTest, FromProto) {
    FloatArray val{};
    FloatArrayParam param(val, pd_);
    st2138::Value protoValue;
    for (float i : value_) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }
    rc_ = param.fromProto(protoValue, authz_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), value_);
}
/*
 * TEST 12 - Testing <FLOAT_ARRAY>ParamWithValue.ValidateSetValue().
 */
TEST_F(ParamWithFloatArrayTest, ValidateSetValue) {
    FloatArrayParam param(value_, pd_);
    st2138::Value protoValue;
    for (float i : {0, 1, 2}) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
}
/*
 * TEST 13 - Testing <FLOAT_ARRAY>ParamWithValue.ValidateSetValue() for
 * appending and setting a single element.
 */
TEST_F(ParamWithFloatArrayTest, ValidateSetValue_SingleElement) {
    FloatArrayParam param(value_, pd_);
    st2138::Value protoValue;
    protoValue.set_float32_value(3);
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_))
        << "Failed setting existing value test.";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Failed appending value test.";
}
/*
 * TEST 14 - Testing <FLOAT_ARRAY>ParamWithValue.ValidateSetValue() error handling.
 * Two main error cases:
 *  - Index is defined for non-element setValue.
 *  - New value exceeds maxLength / validFromProto error.
 */
TEST_F(ParamWithFloatArrayTest, ValidateSetValue_Error) {
    FloatArrayParam param(value_, pd_);
    st2138::Value protoValue;
    for (float i : {0, 1, 2, 3}) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }
    // Defined index with non-single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for typeA -> typeA SetValue.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for typeA -> typeA SetValue.";
    // New value exceeds maxLength / validFromProto error
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value_.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "In this case validFromProto should fail from the array exceeding the max length.";
}
/*
 * TEST 15 - Testing <FLOAT_ARRAY>ParamWithValue.ValidateSetValue() error handling
 * for appending and setting a single element.
 * Four main error cases:
 *  - Index is not defined for single element setValue.
 *  - Index is not kEnd and is out of bounds.
 *  - Type mismatch between proto value and element value / validFromProto error.
 *  - Append would cause array to exceed the max length.
 */
TEST_F(ParamWithFloatArrayTest, ValidateSetValue_SingleElementError) {
    FloatArrayParam param(value_, pd_);
    st2138::Value protoValue;
    protoValue.set_float32_value(3);
    // Index is not defined for single element setValue.
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when index is not defined for single element setValue.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is not defined for single element setValue.";
    // Defined index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value_.size(), authz_, rc_))
        << "ValidateSetValue should return false when index is out of the bounds of the array.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "ValidateSetValue should return OUT_OF_RANGE when index is out of the bounds of the array.";
    // Type mismatch / validFromProto error
    st2138::Value wrongTypeValue;
    wrongTypeValue.set_int32_value(48);
    EXPECT_FALSE(param.validateSetValue(wrongTypeValue, 0, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "In this case validFromProto should fail from a type mismatch.";
    // Too many appends
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value_.size() + 2));
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Param should still be able to append at 2 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Param should still be able to append at 2 elements";
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "ValidateSetValue should return false when appending would exceed the max length.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "ValidateSetValue should return OUT_OF_RANGE when appending would exceed the max length.";
}
