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
 * @brief This file is for testing the <STRING>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/08/07
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;
using StringParam = ParamWithValue<std::string>;

// Fixture
class ParamWithStringTest : public ParamTest<std::string> {
  protected:
    /*
     * Returns the value type of the parameter we are testing with.
     */
    catena::ParamType type() const override { return catena::ParamType::STRING; }

    std::string value_{"Hello World"};
};

/*
 * TEST 1 - Testing <STRING>ParamWithValue constructors.
 */
TEST_F(ParamWithStringTest, Create) {
    CreateTest(value_);
}
/*
 * TEST 2 - Testing <STRING>ParamWithValue.get().
 */
TEST_F(ParamWithStringTest, Get) {
    GetValueTest(value_);
}
/*
 * TEST 3 - Testing <STRING>ParamWithValue.size().
 */
TEST_F(ParamWithStringTest, Size) {
    StringParam param(value_, pd_);
    EXPECT_EQ(param.size(), value_.size());
}
/*
 * TEST 4 - Testing <STRING>ParamWithValue.getParam().
 * STRING params have no sub-params and should return an error.
 */
TEST_F(ParamWithStringTest, GetParam) {
    StringParam param(value_, pd_);
    Path path = Path("/test/oid");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 5 - Testing <STRING>ParamWithValue.addBack().
 * STRING params are not arrays, so this should return an error.
 */
TEST_F(ParamWithStringTest, AddBack) {
    StringParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_FALSE(addedParam) << "Added a value to non-array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 6 - Testing <STRING>ParamWithValue.popBack().
 * STRING params are not arrays, so this should return an error.
 */
TEST_F(ParamWithStringTest, PopBack) {
    StringParam param(value_, pd_);
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 7 - Testing <STRING>ParamWithValue.toProto().
 */
TEST_F(ParamWithStringTest, ParamToProto) {
    StringParam param(value_, pd_);
    catena::Param outParam;
    rc_ = param.toProto(outParam, authz_);
    // Checking results.
    ASSERT_TRUE(outParam.value().has_string_value());
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value_, outParam.value().string_value());
    EXPECT_EQ(oid_, outParam.template_oid());
}
/*
 * TEST 8 - Testing <STRING>ParamWithValue.fromProto().
 */
TEST_F(ParamWithStringTest, FromProto) {
    StringParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Goodbye, World");
    rc_ = param.fromProto(protoValue, authz_);
    // Checking results.
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(param.get(), protoValue.string_value());
}
/*
 * TEST 9 - Testing <STRING>ParamWithValue.ValidateSetValue().
 */
TEST_F(ParamWithStringTest, ValidateSetValue) {
    StringParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Goodbye, World");
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_));
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/*
 * TEST 10 - Testing <STRING>ParamWithValue.ValidateSetValue() error handling.
 * Two main error cases:
 *  - Index is defined.
 *  - String length exceeds max size (or validFromProto returns false).
 */
TEST_F(ParamWithStringTest, ValidateSetValue_Error) {
    StringParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Goodbye, World");
    // Defined index w non-array
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "ValidateSetValue should return false when index is defined for typeA -> typeA SetValue.";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "ValidateSetValue should return INVALID_ARGUMENT when index is defined for typeA -> typeA SetValue.";
    // ValidFromProto error (max size exceeded)
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(value_.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "ValidateSetValue should return false when validFromProto returns false.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "In this case validFromProto should fail from the string exceeding the max length.";
}
