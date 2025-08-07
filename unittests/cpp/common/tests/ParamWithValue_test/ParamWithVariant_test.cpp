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
 * @brief This file is for testing the <std::variant>ParamWithValue class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"
#include "CommonTestHelpers.h"

using namespace catena::common;

using VariantParam = ParamWithValue<TestVariantStruct>;

class ParamWithVariantTest : public ParamTest<TestVariantStruct> {
  protected:
    catena::ParamType type() const override { return catena::ParamType::STRUCT_VARIANT; }

    TestVariantStruct value_{TestStruct1{.f1{16}, .f2{32}}};
};

/**
 * TEST 1 - Testing <std::variant>ParamWithValue constructors
 */
TEST_F(ParamWithVariantTest, Create) {
    CreateTest(value_);
}

/**
 * TEST 2 - Testing <std::variant>ParamWithValue.get()
 */
TEST_F(ParamWithVariantTest, Get) {
    GetValueTest(value_);
}

TEST_F(ParamWithVariantTest, Size) {
    VariantParam param(value_, pd_);
    EXPECT_EQ(param.size(), 0);
}

TEST_F(ParamWithVariantTest, GetParam) {
    VariantParam param(value_, pd_);
    Path path = Path("/TestStruct1");
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(subpd1_));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f1, std::get<TestStruct1>(value_).f1);
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f2, std::get<TestStruct1>(value_).f2);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd1_);
}
