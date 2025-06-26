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
 * @brief This file is for testing the StructInfo.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Mock objects
#include <mocks/MockParamDescriptor.h>
#include <mocks/MockConstraint.h>

// common
#include "StructInfo.h"

using namespace catena::common;

class StructInfoTest : public ::testing::Test {
  protected:
    catena::Value val_;
    MockParamDescriptor pd_;
    MockConstraint constraint_;
    catena::Value constrainedVal_;
};

/* ============================================================================
 *                                   EMPTY
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, EmptyToProto) {
    toProto(val_, &emptyValue, pd_, Authorizer::kAuthzDisabled);
    EXPECT_TRUE(val_.SerializeAsString().empty());
}

TEST_F(StructInfoTest, EmptyFromProto) {
    fromProto(val_, &emptyValue, pd_, Authorizer::kAuthzDisabled);
}

/* ============================================================================
 *                                  INT32_t
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, IntToProto) {
    int32_t src = 64;
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.int32_value(), src);
}

TEST_F(StructInfoTest, IntFromProto) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.int32_value());
}

TEST_F(StructInfoTest, IntFromProtoConstraint) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.int32_value());
}

TEST_F(StructInfoTest, IntFromProtoRange) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    constrainedVal_.set_int32_value(32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(constraint_, apply(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, constrainedVal_.int32_value());
}

TEST_F(StructInfoTest, IntFromProtoNoInt) {
    int32_t dst = 64;
    val_.set_string_value("Not an int");
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.int32_value());
}

TEST_F(StructInfoTest, IntFromProtoUnsatisfied) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).Times(1).WillOnce(testing::Return(false));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.int32_value());
}

/* ============================================================================
 *                                   FLOAT
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, FloatToProto) {
    float src = 64.64;
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.float32_value(), src);
}

TEST_F(StructInfoTest, FloatFromProto) {
    float dst = 0;
    val_.set_float32_value(64.64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.float32_value());
}

TEST_F(StructInfoTest, FloatFromProtoConstraint) {
    float dst = 0;
    val_.set_float32_value(64.64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.float32_value());
}

TEST_F(StructInfoTest, FloatFromProtoRange) {
    float dst = 0;
    val_.set_float32_value(64.64);
    constrainedVal_.set_float32_value(32.32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    EXPECT_CALL(constraint_, apply(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, constrainedVal_.float32_value());
}

TEST_F(StructInfoTest, FloatFromProtoNoFloat) {
    float dst = 64.64;
    val_.set_string_value("Not a float");
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.float32_value());
}

/* ============================================================================
 *                                   STRING
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, StringToProto) {
    std::string src = "Hello, World!";
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.string_value(), src);
}

TEST_F(StructInfoTest, StringFromProto) {
    std::string dst = "";
    val_.set_string_value("Hello, World!");
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.string_value());
}

TEST_F(StructInfoTest, StringFromProtoConstraint) {
    std::string dst = "";
    val_.set_string_value("Hello, World!");
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.string_value());
}

TEST_F(StructInfoTest, StringFromProtoUnsatisfied) {
    std::string dst = "";
    val_.set_string_value("Hello, World!");
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.string_value());
}

TEST_F(StructInfoTest, StringFromProtoNoString) {
    std::string dst = "Hello, World!";
    val_.set_int32_value(64.64); // Not a string
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.string_value());
}

/* ============================================================================
 *                              VECTOR<INT32_T>
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, IntVectorToProto) {
    std::vector<int32_t> src = {1, 2, 3, 4, 5};
    val_.mutable_int32_array_values()->add_ints(9); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.int32_array_values().ints()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.int32_array_values().ints_size(), src.size());
}

/* ============================================================================
 *                               VECTOR<FLOAT>
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, FloatVectorToProto) {
    std::vector<float> src = {1.1, 2.2, 3.3, 4.4, 5.5};
    val_.mutable_float32_array_values()->add_floats(9.9); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.float32_array_values().floats()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.float32_array_values().floats_size(), src.size());
}

/* ============================================================================
 *                               VECTOR<STRING>
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, StringVectorToProto) {
    std::vector<std::string> src = {"first", "second", "third"};
    val_.mutable_string_array_values()->add_strings("last"); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.string_array_values().strings()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.string_array_values().strings_size(), src.size());
}

