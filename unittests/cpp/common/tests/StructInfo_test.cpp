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
#include "Enums.h"

using namespace catena::common;

class StructInfoTest : public ::testing::Test {
  protected:
    catena::Value val_;
    MockParamDescriptor pd_, subpd1_, subpd2_;
    std::unordered_map<std::string, MockParamDescriptor&> subParams_ {
        {"f1", subpd1_},
        {"f2", subpd2_}
    };
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
 *                                 INT ARRAY
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, IntArrayToProto) {
    std::vector<int32_t> src = {1, 2, 3, 4, 5};
    val_.mutable_int32_array_values()->add_ints(9); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.int32_array_values().ints()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.int32_array_values().ints_size(), src.size());
}

TEST_F(StructInfoTest, IntArrayFromProto) {
    std::vector<int32_t> dst = {9}; // Make sure it clears
    auto array = val_.mutable_int32_array_values();
    for (auto i : {1, 2, 3, 4, 5}) {
        array->add_ints(i);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.int32_array_values().ints()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
}

TEST_F(StructInfoTest, IntArrayFromProtoConstraint) {
    std::vector<int32_t> dst = {9}; // Make sure it clears
    auto array = val_.mutable_int32_array_values();
    for (auto i : {1, 2, 3, 4, 5}) {
        array->add_ints(i);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_int32_array_values()->ints_size();
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.int32_array_values().ints()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
}

TEST_F(StructInfoTest, IntArrayFromProtoRange) {
    std::vector<int32_t> dst = {9}; // Make sure it clears
    auto array = val_.mutable_int32_array_values();
    for (auto i : {1, 2, 3, 4, 5}) {
        array->add_ints(i);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_int32_array_values()->ints_size();
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).Times(times).WillRepeatedly(testing::Return(true));
    // Contraint sets non-odd numbers to 0.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const catena::Value &src) {
            catena::Value ans;
            ans.set_int32_value(src.int32_value() % 2 == 1 ? src.int32_value() : 0);
            constrainedVal_.mutable_int32_array_values()->add_ints(ans.int32_value());
            return ans;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : constrainedVal_.int32_array_values().ints()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), constrainedVal_.int32_array_values().ints_size());
}

TEST_F(StructInfoTest, IntArrayFromProtoNoIntArray) {
    std::vector<int32_t> exp = {9};
    std::vector<int32_t> dst(exp.begin(), exp.end());
    val_.set_string_value("Not an int vector");
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}

TEST_F(StructInfoTest, IntArrayFromProtoUnsatisfied) {
    std::vector<int32_t> dst = {9}; // Make sure it clears
    auto array = val_.mutable_int32_array_values();
    for (auto i : {1, 2, 3, 4, 5}) {
        array->add_ints(i);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_TRUE(dst.empty());
}

/* ============================================================================
 *                                FLOAT ARRAY
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, FloatArrayToProto) {
    std::vector<float> src = {1.1, 2.2, 3.3, 4.4, 5.5};
    val_.mutable_float32_array_values()->add_floats(9.9); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.float32_array_values().floats()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.float32_array_values().floats_size(), src.size());
}

TEST_F(StructInfoTest, FloatArrayFromProto) {
    std::vector<float> dst = {9.9}; // Make sure it clears
    auto array = val_.mutable_float32_array_values();
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        array->add_floats(f);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.float32_array_values().floats()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.float32_array_values().floats_size());
}

TEST_F(StructInfoTest, FloatArrayFromProtoConstraint) {
    std::vector<float> dst = {9.9}; // Make sure it clears
    auto array = val_.mutable_float32_array_values();
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        array->add_floats(f);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_float32_array_values()->floats_size();
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.float32_array_values().floats()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.float32_array_values().floats_size());
}

TEST_F(StructInfoTest, FloatArrayFromProtoRange) {
    std::vector<float> dst = {9.9}; // Make sure it clears
    auto array = val_.mutable_float32_array_values();
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        array->add_floats(f);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_float32_array_values()->floats_size();
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(false));
    // Contraint caps numbers at 3.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const catena::Value &src) {
            catena::Value ans;
            ans.set_float32_value(src.float32_value() < 3 ? src.float32_value() : 3);
            constrainedVal_.mutable_float32_array_values()->add_floats(ans.float32_value());
            return ans;
        }));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : constrainedVal_.float32_array_values().floats()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), constrainedVal_.float32_array_values().floats_size());
}

TEST_F(StructInfoTest, FloatArrayFromProtoNoFloatArray) {
    std::vector<float> exp = {9.9};
    std::vector<float> dst(exp.begin(), exp.end());
    val_.set_string_value("Not a float vector");
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}

/* ============================================================================
 *                               STRING ARRAY
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, StringArrayToProto) {
    std::vector<std::string> src = {"first", "second", "third"};
    val_.mutable_string_array_values()->add_strings("last"); // Make sure it clears
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.string_array_values().strings()) {
        EXPECT_TRUE(std::find(src.begin(), src.end(), i) != src.end());
    }
    EXPECT_EQ(val_.string_array_values().strings_size(), src.size());
}

TEST_F(StructInfoTest, StringArrayFromProto) {
    std::vector<std::string> dst = {"Hello, world!"}; // Make sure it clears
    auto array = val_.mutable_string_array_values();
    for (auto s : {"first", "second", "third"}) {
        array->add_strings(s);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(nullptr));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.string_array_values().strings()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.string_array_values().strings_size());
}

TEST_F(StructInfoTest, StringArrayFromProtoConstraint) {
    std::vector<std::string> dst = {"Hello, world!"}; // Make sure it clears
    auto array = val_.mutable_string_array_values();
    for (auto s : {"first", "second", "third"}) {
        array->add_strings(s);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_string_array_values()->strings_size();
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    for (auto i : val_.string_array_values().strings()) {
        EXPECT_TRUE(std::find(dst.begin(), dst.end(), i) != dst.end());
    }
    EXPECT_EQ(dst.size(), val_.string_array_values().strings_size());
}

TEST_F(StructInfoTest, StringArrayFromProtoNoStringArray) {
    std::vector<std::string> exp = {"Hello, world!"};
    std::vector<std::string> dst(exp.begin(), exp.end());
    val_.set_int32_value(64); // Not a string vector
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}

TEST_F(StructInfoTest, StringArrayFromProtoUnsatisfied) {
    std::vector<std::string> dst = {"Hello, world!"}; // Make sure it clears
    auto array = val_.mutable_string_array_values();
    for (auto s : {"first", "second", "third"}) {
        array->add_strings(s);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    // Calling fromProto()
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_TRUE(dst.empty());
}

/* ============================================================================
 *                                  STRUCT
 * ============================================================================
 * 
 * 
 */

struct TestStruct1 {
    int32_t f1;
    int32_t f2;
    using isCatenaStruct = void;
};
template<>
struct catena::common::StructInfo<TestStruct1> {
    using Type = std::tuple<FieldInfo<int32_t, TestStruct1>, FieldInfo<int32_t, TestStruct1>>;
    static constexpr Type fields = {{"f1", &TestStruct1::f1}, {"f2", &TestStruct1::f2}};
};

struct TestStruct2 {
    float f1;
    float f2;
    using isCatenaStruct = void;
};
template<>
struct catena::common::StructInfo<TestStruct2> {
    using Type = std::tuple<FieldInfo<float, TestStruct2>, FieldInfo<float, TestStruct2>>;
    static constexpr Type fields = {{"f1", &TestStruct2::f1}, {"f2", &TestStruct2::f2}};
};

TEST_F(StructInfoTest, StructToProto) {
    TestStruct1 src{.f1{1}, .f2{2}};
    for (auto& [field, subPd] : subParams_) {
        EXPECT_CALL(pd_, getSubParam(field)).Times(1).WillOnce(::testing::ReturnRef(subPd));
        EXPECT_CALL(subPd, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    }
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.struct_value().fields().at("f1").int32_value(), src.f1);
    EXPECT_EQ(val_.struct_value().fields().at("f2").int32_value(), src.f2);
}

/* ============================================================================
 *                               STRUCT ARRAY
 * ============================================================================
 * 
 * 
 */
TEST_F(StructInfoTest, StructArrayToProto) {
    std::vector<TestStruct1> src = {
        {.f1{1}, .f2{2}},
        {.f1{3}, .f2{4}},
        {.f1{5}, .f2{6}}
    };
    for (auto& [field, subPd] : subParams_) {
        EXPECT_CALL(pd_, getSubParam(field)).WillRepeatedly(::testing::ReturnRef(subPd));
        EXPECT_CALL(subPd, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    }
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.struct_array_values().struct_values_size(), src.size());
    for (size_t i = 0; i < src.size(); i += 1) {
        auto structProto = val_.struct_array_values().struct_values().at(i);
        EXPECT_EQ(structProto.fields().at("f1").int32_value(), src[i].f1);
        EXPECT_EQ(structProto.fields().at("f2").int32_value(), src[i].f2);
    }
}

/* ============================================================================
 *                              VARIANT STRUCT
 * ============================================================================
 * 
 * 
 */
using TestVariantStruct = std::variant<TestStruct1, TestStruct2>;
template<>
inline std::array<const char*, 2> catena::common::alternativeNames<TestVariantStruct>{"TestStruct1", "TestStruct2"};

TEST_F(StructInfoTest, VariantStructToProto) {
    TestVariantStruct src{TestStruct2{.f1{1.1}, .f2{2.2}}};
    std::string variantType = alternativeNames<TestVariantStruct>[src.index()];
    EXPECT_CALL(pd_, getSubParam(variantType)).WillOnce(::testing::ReturnRef(pd_));
    for (auto& [field, subPd] : subParams_) {
        EXPECT_CALL(pd_, getSubParam(field)).Times(1).WillOnce(::testing::ReturnRef(subPd));
        EXPECT_CALL(subPd, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    }
    auto index = src.index();
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.struct_variant_value().struct_variant_type(), variantType);
    EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f1").float32_value(), std::get<TestStruct2>(src).f1);
    EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f2").float32_value(), std::get<TestStruct2>(src).f2);

    // std::visit([&](auto& arg){
    //     EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f1").float32_value(), arg.f1);
    //     EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f2").float32_value(), arg.f2);
    // }, src);
}

