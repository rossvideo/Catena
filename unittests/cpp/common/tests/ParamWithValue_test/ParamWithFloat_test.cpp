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
 * @brief This file is for testing the <float>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;

using FloatParam = ParamWithValue<float>;

class ParamWithFloatTest : public ParamTest<float> {
  protected:
    catena::ParamType type() const override { return catena::ParamType::FLOAT32; }

    float value_{16};
};

TEST_F(ParamWithFloatTest, Create) {
    CreateTest(value_);
}

TEST_F(ParamWithFloatTest, Get) {
    GetValueTest(value_);
}

TEST_F(ParamWithFloatTest, Size) {
    FloatParam param(value_, pd_);
    EXPECT_EQ(param.size(), 0);
}

TEST_F(ParamWithFloatTest, GetParam) {
    FloatParam param(value_, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithFloatTest, AddBack) {
    FloatParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithFloatTest, PopBack) {
    FloatParam param(value_, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ParamWithFloatTest, ParamToProto) {
    FloatParam param(value_, pd_);
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
    EXPECT_EQ(value_, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

TEST_F(ParamWithFloatTest, FromProto) {
    FloatParam param(value_, pd_);
    float newValue{16};
    catena::Value protoValue;
    protoValue.set_float32_value(newValue);
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), newValue);
}

TEST_F(ParamWithFloatTest, ValidateSetValue) {
    FloatParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_float32_value(16);
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithFloatTest, ValidateSetValue_Error) {
    FloatParam param(value_, pd_);
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
