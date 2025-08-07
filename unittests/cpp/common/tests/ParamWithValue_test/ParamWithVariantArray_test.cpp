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
 * @brief This file is for testing the <std::vector<std::variant>>ParamWithValue
 * class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"
#include "CommonTestHelpers.h"

using namespace catena::common;

using VariantArray = std::vector<TestVariantStruct>;
using VariantArrayParam = ParamWithValue<VariantArray>;

class ParamWithVariantArrayTest : public ParamTest<VariantArray> {
  protected:
    catena::ParamType type() const override { return catena::ParamType::STRUCT_VARIANT_ARRAY; }

    VariantArray value_{TestStruct1{1, 2}, TestStruct2{3.3, 4.4}, TestStruct1{5, 6}};
};

/**
 * TEST 1 - Testing <std::vector<std::variant>>ParamWithValue constructors
 */
TEST_F(ParamWithVariantArrayTest, Create) {
    CreateTest(value_);
}

/**
 * TEST 2 - Testing <std::vector<std::variant>>ParamWithValue.get()
 */
TEST_F(ParamWithVariantArrayTest, Get) {
    GetValueTest(value_);
}

TEST_F(ParamWithVariantArrayTest, Size) {
    VariantArrayParam param(value_, pd_);
    EXPECT_EQ(param.size(), value_.size());
}
