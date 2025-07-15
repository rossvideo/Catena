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

/*
 * A test CatenaStruct with two int fields.
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
/*
 * A test CatenaStruct with two float fields.
 */
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
/*
 * A test CatenaStruct variant which can be a TestStruct1 or a TestStruct2.
 */
using TestVariantStruct = std::variant<TestStruct1, TestStruct2>;
template<>
inline std::array<const char*, 2> catena::common::alternativeNames<TestVariantStruct>{"TestStruct1", "TestStruct2"};

// Fixture
class StructInfoTest : public ::testing::Test {
  protected:
    /*
     * Sets up default expectations for pd_.
     */
    void SetUp() override {
        EXPECT_CALL(pd_, getSubParam(testing::_)).WillRepeatedly(::testing::ReturnRef(pd_));
        EXPECT_CALL(pd_, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
        EXPECT_CALL(pd_, readOnly()).WillRepeatedly(testing::Return(false));
        EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(nullptr));
        EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
        EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(20));
        EXPECT_CALL(pd_, getOid()).WillRepeatedly(testing::ReturnRef(oid));
        EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    }
    /*
     * Helper function to initialize arrays of TestStruct1s for TEST 6
     */
    void initVal_(std::vector<TestStruct1> array) {
        for (auto& testStruct : array) {
            auto newStruct = val_.mutable_struct_array_values()->add_struct_values();
            catena::Value fieldVal;
            fieldVal.set_int32_value(testStruct.f1);
            newStruct->mutable_fields()->insert({"f1", fieldVal});
            fieldVal.set_int32_value(testStruct.f2);
            newStruct->mutable_fields()->insert({"f2", fieldVal});
        }
    }
    /*
     * Helper function to initialize arrays of TestVariantStructs for TEST 8
     */
    void initVal_(std::vector<TestVariantStruct> array) {
        for (auto& testVal : array) {
            std::string variantType = alternativeNames<TestVariantStruct>[testVal.index()];
            auto newStruct = val_.mutable_struct_variant_array_values()->add_struct_variants();
            catena::Value f1, f2;
            if (variantType == "TestStruct1") {
                f1.set_int32_value(std::get<TestStruct1>(testVal).f1);
                f2.set_int32_value(std::get<TestStruct1>(testVal).f2);
            } else if (variantType == "TestStruct2") {
                f1.set_float32_value(std::get<TestStruct2>(testVal).f1);
                f2.set_float32_value(std::get<TestStruct2>(testVal).f2);
            }
            newStruct->set_struct_variant_type(variantType);
            newStruct->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
            newStruct->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
        }
    }
    /*
     * Helper function to compare results for TEST 6
     */
    void cmpVal_(const std::vector<TestStruct1>& cmpVal) const {
        EXPECT_EQ(cmpVal.size(), val_.struct_array_values().struct_values_size());
        for (size_t i = 0; i < cmpVal.size(); i += 1) {
            auto& structProto = val_.struct_array_values().struct_values().at(i);
            EXPECT_EQ(cmpVal[i].f1, structProto.fields().at("f1").int32_value());
            EXPECT_EQ(cmpVal[i].f2, structProto.fields().at("f2").int32_value());
        }
    }
    /*
     * Helper function to compare results for TEST 8
     */
    void cmpVal_(const std::vector<TestVariantStruct>& cmpVal) const {
        EXPECT_EQ(val_.struct_variant_array_values().struct_variants_size(), cmpVal.size());
        for (size_t i = 0; i < cmpVal.size(); i += 1) {
            auto& testStruct = cmpVal[i];
            std::string variantType = alternativeNames<TestVariantStruct>[testStruct.index()];
            auto structProto = val_.struct_variant_array_values().struct_variants().at(i);
            EXPECT_EQ(structProto.struct_variant_type(), variantType);
            if (variantType == "TestStruct1") {
                EXPECT_EQ(structProto.value().struct_value().fields().at("f1").int32_value(), std::get<TestStruct1>(testStruct).f1);
                EXPECT_EQ(structProto.value().struct_value().fields().at("f2").int32_value(), std::get<TestStruct1>(testStruct).f2);
            } else if (variantType == "TestStruct2") {
                EXPECT_EQ(structProto.value().struct_value().fields().at("f1").float32_value(), std::get<TestStruct2>(testStruct).f1);
                EXPECT_EQ(structProto.value().struct_value().fields().at("f2").float32_value(), std::get<TestStruct2>(testStruct).f2);
            }
        }
    }

    std::string oid = "test_oid";

    catena::Value val_;
    MockParamDescriptor pd_, subpd1_, subpd2_;
    MockConstraint constraint_;
    catena::Value constrainedVal_;
};



/* ============================================================================
 *                                   EMPTY
 * ============================================================================
 * 
 * TEST 0.1 - Empty ToProto
 */
TEST_F(StructInfoTest, EmptyToProto) {
    toProto(val_, &emptyValue, pd_, Authorizer::kAuthzDisabled);
    EXPECT_TRUE(val_.SerializeAsString().empty());
}
/*
 * TEST 0.2 - Empty FromProto
 */
TEST_F(StructInfoTest, EmptyFromProto) {
    fromProto(val_, &emptyValue, pd_, Authorizer::kAuthzDisabled);
}



/* ============================================================================
 *                                  INT32_t
 * ============================================================================
 * 
 * TEST 1.1 - Int ToProto
 */
TEST_F(StructInfoTest, IntToProto) {
    int32_t src = 64;
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.int32_value(), src);
}
/*
 * TEST 1.2 - Int FromProto
 */
TEST_F(StructInfoTest, IntFromProto) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.int32_value());
}
/*
 * TEST 1.3 - Int FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, IntFromProtoConstraint) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.int32_value());
}
/*
 * TEST 1.4 - Int FromProto with range constraint
 */
TEST_F(StructInfoTest, IntFromProtoRange) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    constrainedVal_.set_int32_value(32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).Times(2).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(constraint_, apply(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, constrainedVal_.int32_value());
}
/*
 * TEST 1.5 - Int FromProto with no int value
 */
TEST_F(StructInfoTest, IntFromProtoNoInt) {
    int32_t dst = 64;
    val_.set_string_value("Not an int");
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.int32_value());
}
/*
 * TEST 1.6 - Int FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, IntFromProtoUnsatisfied) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    EXPECT_CALL(constraint_, isRange()).Times(1).WillOnce(testing::Return(false));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.int32_value());
}



/* ============================================================================
 *                                   FLOAT
 * ============================================================================
 * 
 * TEST 2.1 - Float ToProto
 */
TEST_F(StructInfoTest, FloatToProto) {
    // Initializing in/out variables.
    float src = 64.64;
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.float32_value(), src);
}
/*
 * TEST 2.2 - Float FromProto
 */
TEST_F(StructInfoTest, FloatFromProto) {
    // Initializing in/out variables.
    float dst = 0;
    val_.set_float32_value(64.64);
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.float32_value());
}
/*
 * TEST 2.3 - Float FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, FloatFromProtoConstraint) {
    // Initializing in/out variables.
    float dst = 0;
    val_.set_float32_value(64.64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.float32_value());
}
/*
 * TEST 2.4 - Float FromProto with range constraint
 */
TEST_F(StructInfoTest, FloatFromProtoRange) {
    // Initializing in/out variables.
    float dst = 0;
    val_.set_float32_value(64.64);
    constrainedVal_.set_float32_value(32.32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).Times(2).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(constraint_, apply(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, constrainedVal_.float32_value());
}
/*
 * TEST 2.5 - Float FromProto with no float value
 */
TEST_F(StructInfoTest, FloatFromProtoNoFloat) {
    // Initializing in/out variables.
    float dst = 64.64;
    val_.set_string_value("Not a float");
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.float32_value());
}



/* ============================================================================
 *                                   STRING
 * ============================================================================
 * 
 * TEST 3.1 - String ToProto
 */
TEST_F(StructInfoTest, StringToProto) {
    // Initializing in/out variables.
    std::string src = "Hello";
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.string_value(), src);
}
/*
 * TEST 3.2 - String FromProto
 */
TEST_F(StructInfoTest, StringFromProto) {
    // Initializing in/out variables.
    std::string dst = "";
    val_.set_string_value("Hello");
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.string_value());
}
/*
 * TEST 3.3 - String FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, StringFromProtoConstraint) {
    // Initializing in/out variables.
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](const catena::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, val_.string_value());
}
/*
 * TEST 3.4 - String FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, StringFromProtoUnsatisfied) {
    // Initializing in/out variables.
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(1).WillOnce(testing::Return(false));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.string_value());
}
/*
 * TEST 3.5 - String FromProto with no string value
 */
TEST_F(StructInfoTest, StringFromProtoNoString) {
    // Initializing in/out variables.
    std::string dst = "Hello";
    val_.set_int32_value(64.64); // Not a string
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_NE(dst, val_.string_value());
}



/* ============================================================================
 *                                 INT ARRAY
 * ============================================================================
 * 
 * TEST 4.1 - Int Array ToProto
 */
TEST_F(StructInfoTest, IntArrayToProto) {
    // Initializing in/out variables.
    std::vector<int32_t> src = {1, 2, 3, 4, 5};
    val_.mutable_int32_array_values()->add_ints(9); 
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(src.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.int32_array_values().ints().at(i));
    }
}
/* 
 * TEST 4.2 - Int Array FromProto
 */
TEST_F(StructInfoTest, IntArrayFromProto) {
    // Initializing in/out variables.
    std::vector<int32_t> dst = {9}; 
    for (auto i : {1, 2, 3, 4, 5}) {
        val_.mutable_int32_array_values()->add_ints(i);
    }
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.int32_array_values().ints().at(i));
    }
}
/* 
 * TEST 4.3 - Int Array FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, IntArrayFromProtoConstraint) {
    // Initializing in/out variables.
    std::vector<int32_t> dst = {9}; 
    for (auto i : {1, 2, 3, 4, 5}) {
        val_.mutable_int32_array_values()->add_ints(i);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_int32_array_values()->ints_size();
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.int32_array_values().ints().at(i));
    }
}
/* 
 * TEST 4.4 - Int Array FromProto with range constraint
 */
TEST_F(StructInfoTest, IntArrayFromProtoRange) {
    // Initializing in/out variables.
    std::vector<int32_t> dst = {9}; 
    for (auto i : {1, 2, 3, 4, 5}) {
        val_.mutable_int32_array_values()->add_ints(i);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_int32_array_values()->ints_size();
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).Times(times + 1).WillRepeatedly(testing::Return(true));
    // Contraint sets non-odd numbers to 0.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const catena::Value &src) {
            catena::Value ans;
            ans.set_int32_value(src.int32_value() % 2 == 1 ? src.int32_value() : 0);
            constrainedVal_.mutable_int32_array_values()->add_ints(ans.int32_value());
            return ans;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    val_.CopyFrom(constrainedVal_);
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.int32_array_values().ints().at(i));
    }
}
/* 
 * TEST 4.5 - Int Array FromProto with no int array value
 */
TEST_F(StructInfoTest, IntArrayFromProtoNoIntArray) {
    // Initializing in/out variables.
    std::vector<int32_t> exp = {9};
    std::vector<int32_t> dst(exp.begin(), exp.end());
    val_.set_string_value("Not an int vector");
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}
/* 
 * TEST 4.6 - Int Array FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, IntArrayFromProtoUnsatisfied) {
    // Initializing in/out variables.
    std::vector<int32_t> dst = {9};
    for (auto i : {1, 2, 3, 4, 5}) {
        val_.mutable_int32_array_values()->add_ints(i);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, (std::vector<int32_t>){9});
}



/* ============================================================================
 *                                FLOAT ARRAY
 * ============================================================================
 * 
 * TEST 5.1 - Float Array ToProto
 */
TEST_F(StructInfoTest, FloatArrayToProto) {
    // Initializing in/out variables.
    std::vector<float> src = {1.1, 2.2, 3.3, 4.4, 5.5};
    val_.mutable_float32_array_values()->add_floats(9.9); 
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(src.size(), val_.float32_array_values().floats_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.float32_array_values().floats().at(i));
    }
}
/*
 * TEST 4.2 - Float Array FromProto
 */
TEST_F(StructInfoTest, FloatArrayFromProto) {
    // Initializing in/out variables.
    std::vector<float> dst = {9.9}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        val_.mutable_float32_array_values()->add_floats(f);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(nullptr));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.float32_array_values().floats_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.float32_array_values().floats().at(i));
    }
}
/*
 * TEST 4.3 - Float Array FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, FloatArrayFromProtoConstraint) {
    // Initializing in/out variables.
    std::vector<float> dst = {9.9}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        val_.mutable_float32_array_values()->add_floats(f);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_float32_array_values()->floats_size();
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.float32_array_values().floats_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.float32_array_values().floats().at(i));
    }
}
/*
 * TEST 4.4 - Float Array FromProto with range constraint
 */
TEST_F(StructInfoTest, FloatArrayFromProtoRange) {
    // Initializing in/out variables.
    std::vector<float> dst = {9.9}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) {
        val_.mutable_float32_array_values()->add_floats(f);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_float32_array_values()->floats_size();
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).Times(times + 1).WillRepeatedly(testing::Return(true));
    // Contraint caps numbers at 3.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const catena::Value &src) {
            catena::Value ans;
            ans.set_float32_value(src.float32_value() < 3 ? src.float32_value() : 3);
            constrainedVal_.mutable_float32_array_values()->add_floats(ans.float32_value());
            return ans;
        }));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), constrainedVal_.float32_array_values().floats_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], constrainedVal_.float32_array_values().floats().at(i));
    }
}
/*
 * TEST 4.5 - Float Array FromProto with no float array value
 */
TEST_F(StructInfoTest, FloatArrayFromProtoNoFloatArray) {
    // Initializing in/out variables.
    std::vector<float> exp = {9.9};
    std::vector<float> dst(exp.begin(), exp.end());
    val_.set_string_value("Not a float vector");
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}

/* ============================================================================
 *                               STRING ARRAY
 * ============================================================================
 * 
 * TEST 4.1 - String Array ToProto
 */
TEST_F(StructInfoTest, StringArrayToProto) {
    // Initializing in/out variables.
    std::vector<std::string> src = {"first", "second", "third"};
    val_.mutable_string_array_values()->add_strings("last");
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(src.size(), val_.string_array_values().strings_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.string_array_values().strings().at(i));
    }
}
/*
 * TEST 4.2 - String Array FromProto
 */
TEST_F(StructInfoTest, StringArrayFromProto) {
    // Initializing in/out variables.
    std::vector<std::string> dst = {"Hello"};
    for (auto s : {"first", "second", "third"}) {
        val_.mutable_string_array_values()->add_strings(s);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(nullptr));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.string_array_values().strings_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.string_array_values().strings().at(i));
    }
}
/*
 * TEST 4.3 - String Array FromProto with satisfied constraint
 */
TEST_F(StructInfoTest, StringArrayFromProtoConstraint) {
    // Initializing in/out variables.
    std::vector<std::string> dst = {"Hello"};
    for (auto s : {"first", "second", "third"}) {
        val_.mutable_string_array_values()->add_strings(s);
    }
    // Setting expectations.
    uint32_t times = val_.mutable_string_array_values()->strings_size();
    EXPECT_CALL(pd_, getConstraint()).Times(2).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).Times(times).WillRepeatedly(testing::Return(true));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.size(), val_.string_array_values().strings_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.string_array_values().strings().at(i));
    }
}
/*
 * TEST 4.4 - String Array FromProto with no string array value
 */
TEST_F(StructInfoTest, StringArrayFromProtoNoStringArray) {
    // Initializing in/out variables.
    std::vector<std::string> exp = {"Hello"};
    std::vector<std::string> dst(exp.begin(), exp.end());
    val_.set_int32_value(64); // Not a string vector
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, exp);
}
/*
 * TEST 4.5 - String Array FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, StringArrayFromProtoUnsatisfied) {
    // Initializing in/out variables.
    std::vector<std::string> dst = {"Hello"};
    for (auto s : {"first", "second", "third"}) {
        val_.mutable_string_array_values()->add_strings(s);
    }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).Times(1).WillOnce(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst, (std::vector<std::string>{"Hello"}));
}

/* ============================================================================
 *                                  STRUCT
 * ============================================================================
 * 
 * TEST 5.1 - Struct ToProto
 */
TEST_F(StructInfoTest, StructToProto) {
    // Initializing in variable.
    TestStruct1 src{.f1{1}, .f2{2}};
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.struct_value().fields().at("f1").int32_value(), src.f1);
    EXPECT_EQ(val_.struct_value().fields().at("f2").int32_value(), src.f2);
}
/* 
 * TEST 5.1 - Struct FromProto
 */
TEST_F(StructInfoTest, StructFromProto) {
    // Initializing in and out variables.
    TestStruct1 dst{.f1{0}, .f2{0}};
    catena::Value f1, f2;
    f1.set_int32_value(1);
    f2.set_int32_value(2);
    val_.mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(dst.f1, val_.struct_value().fields().at("f1").int32_value());
    EXPECT_EQ(dst.f2, val_.struct_value().fields().at("f2").int32_value());
}



/* ============================================================================
 *                               STRUCT ARRAY
 * ============================================================================
 * 
 * TEST 6.1 - Struct Array ToProto
 */
TEST_F(StructInfoTest, StructArrayToProto) {
    // Initializing in variable.
    std::vector<TestStruct1> src = {
        {.f1{1}, .f2{2}},
        {.f1{3}, .f2{4}},
        {.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    cmpVal_(src);
}
/* 
 * TEST 6.2 - Struct Array FromProto
 */
TEST_F(StructInfoTest, StructArrayFromProto) {
    // Initializing in and out variables.
    std::vector<TestStruct1> src = {{.f1{9}, .f2{9}}};
    initVal_((std::vector<TestStruct1>){{.f1{1}, .f2{2}}, {.f1{3}, .f2{4}}, {.f1{5}, .f2{6}}});
    // Calling fromProto() and comparing the result
    fromProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    cmpVal_(src);
}



/* ============================================================================
 *                              VARIANT STRUCT
 * ============================================================================
 * 
 * TEST 7.1 - Variant Struct ToProto
 */
TEST_F(StructInfoTest, VariantStructToProto) {
    // Initializing in variable.
    TestVariantStruct src{TestStruct2{.f1{1.1}, .f2{2.2}}};
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(val_.struct_variant_value().struct_variant_type(), alternativeNames<TestVariantStruct>[src.index()]);
    EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f1").float32_value(), std::get<TestStruct2>(src).f1);
    EXPECT_EQ(val_.struct_variant_value().value().struct_value().fields().at("f2").float32_value(), std::get<TestStruct2>(src).f2);
}
/*
 * TEST 7.2 - Variant Struct FromProto
 */
TEST_F(StructInfoTest, VariantStructFromProto) {
    // Initializing in and out variables.
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    catena::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    EXPECT_EQ(alternativeNames<TestVariantStruct>[dst.index()], val_.struct_variant_value().struct_variant_type());
    EXPECT_EQ(std::get<TestStruct2>(dst).f1, val_.struct_variant_value().value().struct_value().fields().at("f1").float32_value());
    EXPECT_EQ(std::get<TestStruct2>(dst).f2, val_.struct_variant_value().value().struct_value().fields().at("f2").float32_value());
}



/* ============================================================================
 *                           VARIANT STRUCT ARRAY
 * ============================================================================
 * 
 * TEST 8.1 - Variant Struct Array ToProto
 */
TEST_F(StructInfoTest, VariantStructArrayToProto) {
    // Initializing in variable.
    std::vector<TestVariantStruct> src = {
        TestStruct1{.f1{1}, .f2{2}},
        TestStruct2{.f1{3.3}, .f2{4.4}},
        TestStruct1{.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    toProto(val_, &src, pd_, Authorizer::kAuthzDisabled);
    cmpVal_(src);
}
/*
 * TEST 8.2 - Variant Struct Array FromProto
 */
TEST_F(StructInfoTest, VariantStructArrayFromProto) {
    // Initializing in/out variables.
    std::vector<TestVariantStruct> dst = {TestStruct1{.f1{9}, .f2{9}}};
    initVal_({TestStruct1{.f1{1}, .f2{2}}, TestStruct2{.f1{3.3}, .f2{4.4}}, TestStruct1{.f1{5}, .f2{6}}});
    // Calling fromProto() and comparing the result
    fromProto(val_, &dst, pd_, Authorizer::kAuthzDisabled);
    cmpVal_(dst);
}
