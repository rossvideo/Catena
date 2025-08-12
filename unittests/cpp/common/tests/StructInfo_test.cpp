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
#include <mocks/MockAuthorizer.h>

// common
#include "CommonTestHelpers.h"
#include "Enums.h"

using namespace catena::common;

// Fixture
class StructInfoTest : public ::testing::Test {
  protected:
    /*
     * Sets up default expectations for pd_.
     */
    void SetUp() override {
        for (auto& pd : (std::vector<MockParamDescriptor*>){&pd_, &subpd1_, &subpd2_}) {
            EXPECT_CALL(*pd, getSubParam(testing::_)).WillRepeatedly(::testing::ReturnRef(*pd));
            EXPECT_CALL(*pd, getConstraint()).WillRepeatedly(testing::Return(nullptr));
            EXPECT_CALL(*pd, max_length()).WillRepeatedly(testing::Return(5));
            EXPECT_CALL(*pd, total_length()).WillRepeatedly(testing::Return(20));
            EXPECT_CALL(*pd, getOid()).WillRepeatedly(testing::ReturnRef(oid_));
            EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
            EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(*pd)))).WillRepeatedly(testing::Return(true));
        }
        EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    }
    /*
     * Helper function to initialize arrays of TestStruct1s for TEST 6
     */
    void initVal_(std::vector<TestStruct1> array) {
        for (auto& testStruct : array) {
            auto newStruct = val_.mutable_struct_array_values()->add_struct_values();
            st2138::Value fieldVal;
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
            st2138::Value f1, f2;
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

    std::string oid_ = "test_oid";

    catena::exception_with_status rc{"", catena::StatusCode::OK};

    st2138::Value val_;
    MockParamDescriptor pd_, subpd1_, subpd2_;
    MockConstraint constraint_;
    MockAuthorizer authz_;
    st2138::Value constrainedVal_;
};



/* ============================================================================
 *                                   EMPTY
 * ============================================================================
 * 
 * TEST 0.1 - Empty ToProto
 */
TEST_F(StructInfoTest, Empty_ToProto_Normal) {
    rc = toProto(val_, &emptyValue, pd_, authz_);
    EXPECT_TRUE(val_.SerializeAsString().empty());
}
/* 
 * TEST 0.2 - Empty ValidFromProto
 */
TEST_F(StructInfoTest, Empty_ValidFromProto) {
    EXPECT_TRUE(validFromProto(val_, &emptyValue, pd_, rc, authz_));
}
/*
 * TEST 0.3 - Empty FromProto
 */
TEST_F(StructInfoTest, Empty_FromProto) {
    rc = fromProto(val_, &emptyValue, pd_, authz_);
}



/* ============================================================================
 *                                  INT32_t
 * ============================================================================
 * 
 * TEST 1.1 - Int ToProto
 */
TEST_F(StructInfoTest, Int_ToProto_Normal) {
    int32_t src = 64;
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(val_.int32_value(), src);
}
/* 
 * TEST 1.2 - Int ToProto without read authz
 */
TEST_F(StructInfoTest, Int_ToProto_NoAuthz) {
    int32_t src = 64;
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_int32_value());
}
/*
 * TEST 1.3 - Int ValidFromProto
 */
TEST_F(StructInfoTest, Int_ValidFromProto_Normal) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 1.4 - Int ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, Int_ValidFromProto_Constraint) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(
        testing::Invoke([this](const st2138::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 1.5 - Int ValidFromProto with range constraint
 */
TEST_F(StructInfoTest, Int_ValidFromProto_Range) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 1.6 - Int ValidFromProto without write authz
 */
TEST_F(StructInfoTest, Int_ValidFromProto_NoAuthz) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 1.7 - Int ValidFromProto with no int value
 */
TEST_F(StructInfoTest, Int_ValidFromProto_TypeMismatch) {
    int32_t dst = 64;
    val_.set_string_value("Not an int");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 1.8 - Int FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, Int_ValidFromProto_Unsatisfied) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 1.9 - Int FromProto
 */
TEST_F(StructInfoTest, Int_FromProto_Normal) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst, val_.int32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/*
 * TEST 1.10 - Int FromProto with range constraint
 */
TEST_F(StructInfoTest, Int_FromProto_Range) {
    int32_t dst = 0;
    val_.set_int32_value(64);
    constrainedVal_.set_int32_value(32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(constraint_, apply(testing::_)).WillRepeatedly(
        testing::Invoke([this](const st2138::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst, constrainedVal_.int32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}


/* ============================================================================
 *                                   FLOAT
 * ============================================================================
 * 
 * TEST 2.1 - Float ToProto
 */
TEST_F(StructInfoTest, Float_ToProto_Normal) {
    float src = 64.64;
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(val_.float32_value(), src);
}
/* 
 * TEST 2.2 - Float ToProto without read authz
 */
TEST_F(StructInfoTest, Float_ToProto_NoAuthz) {
    float src = 64.64;
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_float32_value());
}
/*
 * TEST 2.3 - Float ValidFromProto
 */
TEST_F(StructInfoTest, Float_ValidFromProto_Normal) {
    float dst = 0;
    val_.set_float32_value(64);
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 2.4 - Float ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, Float_ValidFromProto_Constraint) {
    float dst = 0;
    val_.set_float32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(
        testing::Invoke([this](const st2138::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 2.5 - Float ValidFromProto with range constraint
 */
TEST_F(StructInfoTest, Float_ValidFromProto_Range) {
    float dst = 0;
    val_.set_float32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 2.6 - Float ValidFromProto without write authz
 */
TEST_F(StructInfoTest, Float_ValidFromProto_NoAuthz) {
    float dst = 0;
    val_.set_float32_value(64);
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 2.7 - Float ValidFromProto with no float value
 */
TEST_F(StructInfoTest, Float_ValidFromProto_TypeMismatch) {
    float dst = 0;
    val_.set_string_value("Not an float");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 2.8 - Float FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, Float_ValidFromProto_Unsatisfied) {
    float dst = 0;
    val_.set_float32_value(64);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 2.9 - Float FromProto
 */
TEST_F(StructInfoTest, Float_FromProto_Normal) {
    float dst = 0;
    val_.set_float32_value(64.64);
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst, val_.float32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/*
 * TEST 2.10 - Float FromProto with range constraint
 */
TEST_F(StructInfoTest, Float_FromProto_Range) {
    float dst = 0;
    val_.set_float32_value(64.64);
    constrainedVal_.set_float32_value(32.32);
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(constraint_, apply(testing::_)).WillRepeatedly(
        testing::Invoke([this](const st2138::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return constrainedVal_;
        }));
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst, constrainedVal_.float32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                                   STRING
 * ============================================================================
 * 
 * TEST 3.1 - String ToProto
 */
TEST_F(StructInfoTest, String_ToProto_Normal) {
    // Initializing in/out variables.
    std::string src = "Hello";
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(val_.string_value(), src);
}
/* 
 * TEST 3.2 - String ToProto without read authz
 */
TEST_F(StructInfoTest, String_ToProto_NoAuthz) {
    std::string src = "Hello";
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_string_value());
}
/*
 * TEST 3.3 - String ValidFromProto
 */
TEST_F(StructInfoTest, String_ValidFromProto_Normal) {
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, type()).WillRepeatedly(testing::Return(st2138::ParamType::STRING));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 3.4 - String ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, String_ValidFromProto_Constraint) {
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, type()).WillRepeatedly(testing::Return(st2138::ParamType::STRING));
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(
        testing::Invoke([this](const st2138::Value &src) {
            EXPECT_EQ(val_.SerializeAsString(), src.SerializeAsString());
            return true;
        }));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 3.5 - String ValidFromProto without write authz
 */
TEST_F(StructInfoTest, String_ValidFromProto_NoAuthz) {
    std::string dst = "";
    val_.set_string_value("Hello");
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 3.6 - String ValidFromProto with no string value
 */
TEST_F(StructInfoTest, String_ValidFromProto_TypeMismatch) {
    std::string dst = "";
    val_.set_int32_value(64); // Not a string
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 3.7 - String FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, String_ValidFromProto_Unsatisfied) {
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, type()).WillRepeatedly(testing::Return(st2138::ParamType::STRING));
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 3.8 - String FromProto
 */
TEST_F(StructInfoTest, String_FromProto_Normal) {
    std::string dst = "";
    val_.set_string_value("Hello");
    // Setting expectations.
    EXPECT_CALL(pd_, type()).WillRepeatedly(testing::Return(st2138::ParamType::STRING));
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst, val_.string_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

/* ============================================================================
 *                                 INT ARRAY
 * ============================================================================
 * 
 * TEST 4.1 - Int Array ToProto
 */
TEST_F(StructInfoTest, IntArray_ToProto_Normal) {
    // Initializing in/out variables.
    std::vector<int32_t> src = {1, 2, 3, 4, 5};
    val_.mutable_int32_array_values()->add_ints(9); 
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(src.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.int32_array_values().ints().at(i));
    }
}
/* 
 * TEST 4.2 - Int Array ToProto without read authz
 */
TEST_F(StructInfoTest, IntArray_ToProto_NoAuthz) {
    std::vector<int32_t> src = {1, 2, 3, 4, 5};
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_int32_array_values());
}
/*
 * TEST 4.3 - Int Array ValidFromProto
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_Normal) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 4.4 - Int Array ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_Constraint) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 4.5 - Int Array ValidFromProto with range constraint
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_Range) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 4.6 - Int Array ValidFromProto without write authz
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_NoAuthz) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 4.7 - Int Array ValidFromProto with no int array value
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_TypeMismatch) {
    std::vector<int32_t> dst = {};
    val_.set_string_value("Not an int array");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 4.8 - Int Array ValidFromProto with array > max_length
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_MaxLength) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5, 6}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/*
 * TEST 4.9 - Int Array FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, IntArray_ValidFromProto_Unsatisfied) {
    std::vector<int32_t> dst = {};
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/* 
 * TEST 4.10 - Int Array FromProto
 */
TEST_F(StructInfoTest, IntArray_FromProto_Normal) {
    std::vector<int32_t> dst = {9}; 
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.size(), val_.int32_array_values().ints_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.int32_array_values().ints().at(i));
    }
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/* 
 * TEST 4.11 - Int Array FromProto with range constraint
 */
TEST_F(StructInfoTest, IntArray_FromProtoRange) {
    std::vector<int32_t> dst = {9}; 
    for (auto i : {1, 2, 3, 4, 5}) { val_.mutable_int32_array_values()->add_ints(i); }
    // Setting expectations.
    uint32_t times = val_.mutable_int32_array_values()->ints_size();
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Contraint sets non-odd numbers to 0.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const st2138::Value &src) {
            st2138::Value ans;
            ans.set_int32_value(src.int32_value() % 2 == 1 ? src.int32_value() : 0);
            constrainedVal_.mutable_int32_array_values()->add_ints(ans.int32_value());
            return ans;
        }));
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.size(), constrainedVal_.int32_array_values().ints_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], constrainedVal_.int32_array_values().ints().at(i));
    }
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}


/* ============================================================================
 *                                FLOAT ARRAY
 * ============================================================================
 * 
 * TEST 5.1 - Float Array ToProto
 */
TEST_F(StructInfoTest, FloatArray_ToProto_Normal) {
    // Initializing in/out variables.
    std::vector<float> src = {1.1, 2.2, 3.3, 4.4, 5.5};
    val_.mutable_float32_array_values()->add_floats(9.9); 
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(src.size(), val_.float32_array_values().floats_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.float32_array_values().floats().at(i));
    }
}
/* 
 * TEST 5.2 - Float Array ToProto without read authz
 */
TEST_F(StructInfoTest, FloatArray_ToProto_NoAuthz) {
    std::vector<float> src = {1.1, 2.2, 3.3, 4.4, 5.5};
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_float32_array_values());
}
/*
 * TEST 5.3 - Float Array ValidFromProto
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_Normal) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 5.4 - Float Array ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_Constraint) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 5.5 - Float Array ValidFromProto with range constraint
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_Range) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 5.6 - Float Array ValidFromProto without write authz
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_NoAuthz) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 5.7 - Float Array ValidFromProto with no float array value
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_TypeMismatch) {
    std::vector<float> dst = {}; 
    val_.set_string_value("Not a float array");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 5.8 - Float Array ValidFromProto with array > max_length
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_MaxLength) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5, 6.6}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/*
 * TEST 5.9 - Float Array FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, FloatArray_ValidFromProto_Unsatisfied) {
    std::vector<float> dst = {}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 5.10 - Float Array FromProto
 */
TEST_F(StructInfoTest, FloatArray_FromProto_Normal) {
    std::vector<float> dst = {9.9}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.size(), val_.float32_array_values().floats_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.float32_array_values().floats().at(i));
    }
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/*
 * TEST 5.11 - Float Array FromProto with range constraint
 */
TEST_F(StructInfoTest, FloatArray_FromProto_Range) {
    std::vector<float> dst = {9.9}; 
    for (auto f : {1.1, 2.2, 3.3, 4.4, 5.5}) { val_.mutable_float32_array_values()->add_floats(f); }
    // Setting expectations.
    uint32_t times = val_.mutable_float32_array_values()->floats_size();
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, isRange()).WillRepeatedly(testing::Return(true));
    // Contraint caps numbers at 3.
    EXPECT_CALL(constraint_, apply(testing::_)).Times(times)
        .WillRepeatedly(testing::Invoke([this](const st2138::Value &src) {
            st2138::Value ans;
            ans.set_float32_value(src.float32_value() < 3 ? src.float32_value() : 3);
            constrainedVal_.mutable_float32_array_values()->add_floats(ans.float32_value());
            return ans;
        }));
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.size(), constrainedVal_.float32_array_values().floats_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], constrainedVal_.float32_array_values().floats().at(i));
    }
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                               STRING ARRAY
 * ============================================================================
 * 
 * TEST 6.1 - String Array ToProto
 */
TEST_F(StructInfoTest, StringArray_ToProto_Normal) {
    // Initializing in/out variables.
    std::vector<std::string> src = {"first", "second", "third"};
    val_.mutable_string_array_values()->add_strings("last");
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(src.size(), val_.string_array_values().strings_size());
    for (size_t i = 0; i < src.size(); i += 1) {
        EXPECT_EQ(src[i], val_.string_array_values().strings().at(i));
    }
}
/* 
 * TEST 6.2 - String Array ToProto without read authz
 */
TEST_F(StructInfoTest, StringArray_ToProto_NoAuthz) {
    std::vector<std::string> src = {"first", "second", "third"};
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_string_array_values());
}
/*
 * TEST 6.3 - String Array ValidFromProto
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_Normal) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"first", "second", "third"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 6.4 - String Array ValidFromProto with satisfied constraint
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_Constraint) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"first", "second", "third"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(true));
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 6.5 - String Array ValidFromProto without write authz
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_NoAuthz) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"first", "second", "third"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 6.6 - String Array ValidFromProto with no string array value
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_TypeMismatch) {
    std::vector<std::string> dst = {};
    val_.set_string_value("Not a string array");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 6.7 - String Array ValidFromProto with array > max_length
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_MaxLength) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"1", "2", "3", "4", "5", "6"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/*
 * TEST 6.8 - String Array FromProto with unsatisfied constraint
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_Unsatisfied) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"first", "second", "third"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Setting expectations.
    EXPECT_CALL(pd_, getConstraint()).WillRepeatedly(testing::Return(&constraint_));
    EXPECT_CALL(constraint_, satisfied(testing::_)).WillRepeatedly(testing::Return(false));
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 6.9 - String Array ValidFromProto with array > total_length
 */
TEST_F(StructInfoTest, StringArray_ValidFromProto_TotalLength) {
    std::vector<std::string> dst = {}; 
    for (auto s : {"This string is greater than the defined total_length"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/*
 * TEST 6.10 - String Array FromProto
 */
TEST_F(StructInfoTest, StringArray_FromProto_Normal) {
    std::vector<std::string> dst = {"Hello"};
    for (auto s : {"first", "second", "third"}) { val_.mutable_string_array_values()->add_strings(s); }
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.size(), val_.string_array_values().strings_size());
    for (size_t i = 0; i < dst.size(); i += 1) {
        EXPECT_EQ(dst[i], val_.string_array_values().strings().at(i));
    }
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                                  STRUCT
 * ============================================================================
 * 
 * TEST 7.1 - Struct ToProto
 */
TEST_F(StructInfoTest, Struct_ToProto_Normal) {
    // Initializing in variable.
    TestStruct1 src{.f1{1}, .f2{2}};
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(val_.struct_value().fields().at("f1").int32_value(), src.f1);
    EXPECT_EQ(val_.struct_value().fields().at("f2").int32_value(), src.f2);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/* 
 * TEST 7.2 - Struct ToProto without read authz
 */
TEST_F(StructInfoTest, Struct_ToProto_NoAuthz) {
    TestStruct1 src{.f1{1}, .f2{2}};
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_value());
}
/* 
 * TEST 7.3 - Struct ToProto without read authz for sub param
 */
TEST_F(StructInfoTest, Struct_ToProto_NestedNoAuthz) {
    TestStruct1 src{.f1{1}, .f2{2}};
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_value());
}
/*
 * TEST 7.4 - Struct ValidFromProto
 */
TEST_F(StructInfoTest, Struct_ValidFromProto_Normal) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    st2138::Value f1, f2;
    f1.set_int32_value(1);
    f2.set_int32_value(2);
    val_.mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 7.5 - Struct ValidFromProto without write authz
 */
TEST_F(StructInfoTest, Struct_ValidFromProto_NoAuthz) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    st2138::Value f1, f2;
    f1.set_int32_value(1);
    f2.set_int32_value(2);
    val_.mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/* 
 * TEST 7.6 - Struct ValidFromProto without write authz for sub param
 */
TEST_F(StructInfoTest, Struct_ValidFromProto_NestedNoAuthz) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    st2138::Value f1, f2;
    f1.set_int32_value(1);
    f2.set_int32_value(2);
    val_.mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 7.7 - Struct ValidFromProto with no struct value
 */
TEST_F(StructInfoTest, Struct_ValidFromProto_TypeMismatch) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    val_.set_string_value("Not a struct");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 7.8 - Struct ValidFromProto with unknown fields
 */
TEST_F(StructInfoTest, Struct_ValidFromProto_FieldMismatch) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    st2138::Value f1;
    f1.set_int32_value(1);
    val_.mutable_struct_value()->mutable_fields()->insert({"unknown_field_1", f1});
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 7.9 - Struct FromProto
 */
TEST_F(StructInfoTest, Struct_FromProto_Normal) {
    TestStruct1 dst{.f1{0}, .f2{0}};
    st2138::Value f1, f2;
    f1.set_int32_value(1);
    f2.set_int32_value(2);
    val_.mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(dst.f1, val_.struct_value().fields().at("f1").int32_value());
    EXPECT_EQ(dst.f2, val_.struct_value().fields().at("f2").int32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                               STRUCT ARRAY
 * ============================================================================
 * 
 * TEST 8.1 - Struct Array ToProto
 */
TEST_F(StructInfoTest, StructArrayToProto_Normal) {
    std::vector<TestStruct1> src = {
        {.f1{1}, .f2{2}},
        {.f1{3}, .f2{4}},
        {.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    cmpVal_(src);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/* 
 * TEST 8.2 - Struct Array ToProto without read authz
 */
TEST_F(StructInfoTest, StructArrayToProto_NoAuthz) {
    std::vector<TestStruct1> src = {
        {.f1{1}, .f2{2}},
        {.f1{3}, .f2{4}},
        {.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_array_values());
}
/* 
 * TEST 8.3 - Struct Array ToProto without read authz for member sub param
 */
TEST_F(StructInfoTest, StructArray_ToProto_NestedNoAuthz) {
    std::vector<TestStruct1> src = {
        {.f1{1}, .f2{2}},
        {.f1{3}, .f2{4}},
        {.f1{5}, .f2{6}}
    };
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_array_values());
}
/*
 * TEST 8.4 - Struct Array ValidFromProto
 */
TEST_F(StructInfoTest, StructArray_ValidFromProto_Normal) {
    std::vector<TestStruct1> dst = {};
    initVal_((std::vector<TestStruct1>){{.f1{1}, .f2{2}}, {.f1{3}, .f2{4}}, {.f1{5}, .f2{6}}});
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 8.5 - Struct Array ValidFromProto without write authz
 */
TEST_F(StructInfoTest, StructArray_ValidFromProto_NoAuthz) {
    std::vector<TestStruct1> dst = {};
    initVal_((std::vector<TestStruct1>){{.f1{1}, .f2{2}}, {.f1{3}, .f2{4}}, {.f1{5}, .f2{6}}});
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/* 
 * TEST 8.6 - Struct Array ValidFromProto without write authz for sub param
 */
TEST_F(StructInfoTest, StructArray_ValidFromProto_NestedNoAuthz) {
    std::vector<TestStruct1> dst = {};
    initVal_((std::vector<TestStruct1>){{.f1{1}, .f2{2}}, {.f1{3}, .f2{4}}, {.f1{5}, .f2{6}}});
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 8.7 - Struct Array ValidFromProto with no struct array value
 */
TEST_F(StructInfoTest, StructArray_ValidFromProto_TypeMismatch) {
    std::vector<TestStruct1> dst = {};
    val_.set_string_value("Not a struct array");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 8.8 - Struct Array ValidFromProto with array > max_length
 */
TEST_F(StructInfoTest, StructArray_ValidFromProto_MaxLength) {
    std::vector<TestStruct1> dst = {};
    initVal_((std::vector<TestStruct1>){
        {.f1{1}, .f2{2}},  {.f1{3},  .f2{4}},
        {.f1{5}, .f2{6}},  {.f1{7},  .f2{8}},
        {.f1{9}, .f2{10}}, {.f1{11}, .f2{12}}
    });
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/* 
 * TEST 8.9 - Struct Array FromProto
 */
TEST_F(StructInfoTest, StructArray_FromProto_Normal) {
    // Initializing in and out variables.
    std::vector<TestStruct1> dst = {{.f1{9}, .f2{9}}};
    initVal_((std::vector<TestStruct1>){{.f1{1}, .f2{2}}, {.f1{3}, .f2{4}}, {.f1{5}, .f2{6}}});
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    cmpVal_(dst);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                              VARIANT STRUCT
 * ============================================================================
 * 
 * TEST 9.1 - Variant Struct ToProto
 */
TEST_F(StructInfoTest, VariantStruct_ToProto_Normal) {
    TestStruct1 src{.f1{1}, .f2{2}};
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(val_.struct_value().fields().at("f1").int32_value(), src.f1);
    EXPECT_EQ(val_.struct_value().fields().at("f2").int32_value(), src.f2);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/* 
 * TEST 9.2 - Variant Struct ToProto without read authz
 */
TEST_F(StructInfoTest, VariantStruct_ToProto_NoAuthz) {
    TestVariantStruct src{TestStruct2{.f1{1.1}, .f2{2.2}}};
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_variant_value());
}
/* 
 * TEST 9.3 - Variant Struct ToProto without read authz for sub param
 */
TEST_F(StructInfoTest, VariantStruct_ToProto_NestedNoAuthz) {
    TestVariantStruct src{TestStruct2{.f1{1.1}, .f2{2.2}}};
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(pd_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_variant_value());
}
/*
 * TEST 9.4 - Variant Struct ValidFromProto
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_Normal) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 9.5 - Variant Struct ValidFromProto without write authz
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_NoAuthz) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/* 
 * TEST 9.6 - Variant Struct ValidFromProto without write authz for sub param
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_NestedNoAuthz) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(pd_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 9.7 - Variant Struct ValidFromProto with no struct value
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_TypeMismatch) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    val_.set_string_value("Not a variant struct");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 9.8 - Variant Struct ValidFromProto with unknown fields
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_VariantTypeMismatch) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("unknown_struct");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f1});
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 9.9 - Variant Struct ValidFromProto with unknown fields
 */
TEST_F(StructInfoTest, VariantStruct_ValidFromProto_FieldMismatch) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"unknown_field_1", f1});
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 9.10 - Variant Struct FromProto
 */
TEST_F(StructInfoTest, VariantStruct_FromProto_Normal) {
    TestVariantStruct dst{TestStruct1{.f1{9}, .f2{9}}};
    st2138::Value f1, f2;
    f1.set_float32_value(1.1);
    f2.set_float32_value(2.2);
    val_.mutable_struct_variant_value()->set_struct_variant_type("TestStruct2");
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f1", f1});
    val_.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields()->insert({"f2", f2});
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    EXPECT_EQ(alternativeNames<TestVariantStruct>[dst.index()], val_.struct_variant_value().struct_variant_type());
    EXPECT_EQ(std::get<TestStruct2>(dst).f1, val_.struct_variant_value().value().struct_value().fields().at("f1").float32_value());
    EXPECT_EQ(std::get<TestStruct2>(dst).f2, val_.struct_variant_value().value().struct_value().fields().at("f2").float32_value());
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}



/* ============================================================================
 *                           VARIANT STRUCT ARRAY
 * ============================================================================
 * 
 * TEST 10.1 - Variant Struct Array ToProto
 */
TEST_F(StructInfoTest, VariantStructArrayToProto_Normal) {
    std::vector<TestVariantStruct> src = {
        TestStruct1{.f1{1}, .f2{2}},
        TestStruct2{.f1{3.3}, .f2{4.4}},
        TestStruct1{.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    cmpVal_(src);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
/* 
 * TEST 10.2 - Variant Struct Array ToProto without read authz
 */
TEST_F(StructInfoTest, VariantStructArrayToProto_NoAuthz) {
    std::vector<TestVariantStruct> src = {
        TestStruct1{.f1{1}, .f2{2}},
        TestStruct2{.f1{3.3}, .f2{4.4}},
        TestStruct1{.f1{5}, .f2{6}}
    };
    // Calling toProto() and comparing the result
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_variant_array_values());
}
/* 
 * TEST 10.3 - Variant Struct Array ToProto without read authz for member sub param
 */
TEST_F(StructInfoTest, VariantStructArray_ToProto_NestedNoAuthz) {
    std::vector<TestVariantStruct> src = {
        TestStruct1{.f1{1}, .f2{2}},
        TestStruct2{.f1{3.3}, .f2{4.4}},
        TestStruct1{.f1{5}, .f2{6}}
    };
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(pd_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    rc = toProto(val_, &src, pd_, authz_);
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
    EXPECT_FALSE(val_.has_struct_variant_array_values());
}
/*
 * TEST 10.4 - Variant Struct Array ValidFromProto
 */
TEST_F(StructInfoTest, VariantStructArray_ValidFromProto_Normal) {
    std::vector<TestVariantStruct> dst = {};
    initVal_({TestStruct1{.f1{1}, .f2{2}}, TestStruct2{.f1{3.3}, .f2{4.4}}, TestStruct1{.f1{5}, .f2{6}}});
    // Calling validFromProto() and comparing the result
    EXPECT_TRUE(validFromProto(val_, &dst, pd_, rc, authz_));
}
/*
 * TEST 10.5 - Variant Struct Array ValidFromProto without write authz
 */
TEST_F(StructInfoTest, VariantStructArray_ValidFromProto_NoAuthz) {
    std::vector<TestVariantStruct> dst = {};
    initVal_({TestStruct1{.f1{1}, .f2{2}}, TestStruct2{.f1{3.3}, .f2{4.4}}, TestStruct1{.f1{5}, .f2{6}}});
    // Calling validFromProto() and comparing the result
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/* 
 * TEST 10.6 - Variant Struct Array ValidFromProto without write authz for sub param
 */
TEST_F(StructInfoTest, VariantStructArray_ValidFromProto_NestedNoAuthz) {
    std::vector<TestVariantStruct> dst = {};
    initVal_({TestStruct1{.f1{1}, .f2{2}}, TestStruct2{.f1{3.3}, .f2{4.4}}, TestStruct1{.f1{5}, .f2{6}}});
    // Setting expectations.
    EXPECT_CALL(pd_, getSubParam(testing::_))
        .WillOnce(testing::ReturnRef(pd_))
        .WillOnce(testing::ReturnRef(subpd1_))
        .WillOnce(testing::ReturnRef(subpd2_));
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(subpd2_)))).WillOnce(testing::Return(false));
    // Calling toProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::PERMISSION_DENIED);
}
/*
 * TEST 10.7 - Variant Struct Array ValidFromProto with no struct array value
 */
TEST_F(StructInfoTest, VariantStructArray_ValidFromProto_TypeMismatch) {
    std::vector<TestVariantStruct> dst = {};
    val_.set_string_value("Not a struct array");
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INVALID_ARGUMENT);
}
/*
 * TEST 10.8 - Variant Struct Array ValidFromProto with array > max_length
 */
TEST_F(StructInfoTest, VariantStructArray_ValidFromProto_MaxLength) {
    std::vector<TestVariantStruct> dst = {};
    initVal_({TestStruct1{.f1{1}, .f2{2}},  TestStruct2{.f1{3.3},   .f2{4.4}},
              TestStruct1{.f1{5}, .f2{6}},  TestStruct2{.f1{7.7},   .f2{8.8}},
              TestStruct1{.f1{9}, .f2{10}}, TestStruct2{.f1{11.11}, .f2{12.12}}});
    // Calling validFromProto() and comparing the result
    EXPECT_FALSE(validFromProto(val_, &dst, pd_, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OUT_OF_RANGE);
}
/* 
 * TEST 10.9 - Variant Struct Array FromProto
 */
TEST_F(StructInfoTest, VariantStructArray_FromProto_Normal) {
    std::vector<TestVariantStruct> dst = {TestStruct1{.f1{9}, .f2{9}}};
    initVal_({TestStruct1{.f1{1}, .f2{2}}, TestStruct2{.f1{3.3}, .f2{4.4}}, TestStruct1{.f1{5}, .f2{6}}});
    // Calling fromProto() and comparing the result
    rc = fromProto(val_, &dst, pd_, authz_);
    cmpVal_(dst);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}
