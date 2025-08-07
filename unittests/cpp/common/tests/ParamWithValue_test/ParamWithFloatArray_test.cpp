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
 * @brief This file is for testing the <std::vector<float>>ParamWithValue
 * class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;

using FloatArray = std::vector<float>;
using FloatArrayParam = ParamWithValue<FloatArray>;

class ParamWithFloatArrayTest : public ParamTest<FloatArray> {
  protected:
    FloatArray value_{0, 1, 2};
};

TEST_F(ParamWithFloatArrayTest, Create) {
    CreateTest(value_);
}

TEST_F(ParamWithFloatArrayTest, Get) {
    GetValueTest(value_);
}

TEST_F(ParamWithFloatArrayTest, Size) {
    FloatArrayParam param(value_, pd_);
    EXPECT_EQ(param.size(), value_.size());
}

TEST_F(ParamWithFloatArrayTest, GetParam) {
    FloatArrayParam param(value_, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<float>(foundParam.get()), value_[0]);
}

TEST_F(ParamWithFloatArrayTest, GetParam_Error) {
    FloatArrayParam param(value_, pd_);
    { // Front is not an index.
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "getParam should return INVALID_ARGUMENT if front of path is not an index";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Index out of bounds
    Path path = Path("/" + std::to_string(value_.size()));
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

TEST_F(ParamWithFloatArrayTest, AddBack) {
    FloatArrayParam param(value_, pd_);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithFloatArrayTest, AddBack_Error) {
    FloatArrayParam param(value_, pd_);
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

TEST_F(ParamWithFloatArrayTest, PopBack) {
    FloatArrayParam param(value_, pd_);
    FloatArray valueCopy{value_.begin(), value_.end()};
    rc_ = param.popBack(authz_);
    valueCopy.pop_back();

    EXPECT_EQ(param.get(), valueCopy);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithFloatArrayTest, PopBack_Error) {
    value_ = {};
    FloatArrayParam param(value_, pd_);
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

TEST_F(ParamWithFloatArrayTest, ParamToProto) {
    FloatArrayParam param(value_, pd_);
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
    EXPECT_EQ(value_, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithFloatArrayTest, ValidateSetValue) {
    FloatArrayParam param(value_, pd_);
    catena::Value protoValue;
    for (float i : {0, 1, 2}) {
        protoValue.mutable_float32_array_values()->add_floats(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

TEST_F(ParamWithFloatArrayTest, ValidateSetValue_SingleElement) {
    FloatArrayParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(3);
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

TEST_F(ParamWithFloatArrayTest, ValidateSetValue_Error) {
    FloatArrayParam param(value_, pd_);
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
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value_.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds maxLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds maxLength";
}

TEST_F(ParamWithFloatArrayTest, ValidateSetValue_SingleElementError) {
    FloatArrayParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(3);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));

    // Undefined index with single element set
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the index is undefined for single element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when the index is undefined for single element setValue";

    // Defined index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value_.size(), authz_, rc_))
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