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

TEST_F(ParamWithVariantArrayTest, GetParam) {
    VariantArrayParam param(value_, pd_);
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(subpd1_));
    { // Top level
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter /0 when one was expected";
    EXPECT_EQ(std::get<TestStruct1>(getParamValue<TestVariantStruct>(foundParam.get())).f1, std::get<TestStruct1>(value_[0]).f1);
    EXPECT_EQ(std::get<TestStruct1>(getParamValue<TestVariantStruct>(foundParam.get())).f2, std::get<TestStruct1>(value_[0]).f2);
    } 
    { // Nested
    Path path = Path("/0/TestStruct1");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f1, std::get<TestStruct1>(value_[0]).f1);
    EXPECT_EQ(getParamValue<TestStruct1>(foundParam.get()).f2, std::get<TestStruct1>(value_[0]).f2);
    EXPECT_EQ(&foundParam->getDescriptor(), &subpd1_);
    }
}

TEST_F(ParamWithVariantArrayTest, GetParam_Error) {
    VariantArrayParam param(value_, pd_);
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
        << "getParam should return OUT_OF_RANGE if the index is out of bounds";
    }
    rc_ = catena::exception_with_status{"", catena::StatusCode::OK}; // Reset status
    { // Not authorized.
    Path path = Path("/0");
    EXPECT_CALL(authz_, readAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_FALSE(foundParam) << "Found a parameter when none was expected";
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "getParam should return PERMISSION_DENIED if Authorizer does not have readAuthz";
    }
}

TEST_F(ParamWithVariantArrayTest, AddBack) {
    VariantArrayParam param(value_, pd_);
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithVariantArrayTest, AddBack_Error) {
    VariantArrayParam param(value_, pd_);
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

TEST_F(ParamWithVariantArrayTest, PopBack) {
    VariantArrayParam param(value_, pd_);
    VariantArray valueCopy{value_.begin(), value_.end()};
    rc_ = param.popBack(authz_);
    valueCopy.pop_back();
    ASSERT_EQ(param.get().size(), valueCopy.size());
    EXPECT_EQ(std::get<TestStruct1>(param.get()[0]).f1, std::get<TestStruct1>(valueCopy[0]).f1);
    EXPECT_EQ(std::get<TestStruct2>(param.get()[1]).f1, std::get<TestStruct2>(valueCopy[1]).f1);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

TEST_F(ParamWithVariantArrayTest, PopBack_Error) {
    VariantArray value{};
    VariantArrayParam param(value, pd_);
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

TEST_F(ParamWithVariantArrayTest, ParamToProto) {
    VariantArrayParam param(value_, pd_);
    VariantArray outValue{};
    catena::Param outParam;
    EXPECT_CALL(pd_, getSubParam(testing::_)).WillRepeatedly(testing::ReturnRef(pd_));
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_struct_variant_array_values());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot continue test.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(oid_, outParam.template_oid());
    for (uint32_t i = 0; i < value_.size(); ++i) {
        std::string variantType = alternativeNames<TestVariantStruct>[value_[i].index()];
        EXPECT_EQ(variantType, alternativeNames<TestVariantStruct>[outValue[i].index()]);
        if (variantType == "TestStruct1") {
            EXPECT_EQ(std::get<TestStruct1>(value_[i]).f1, std::get<TestStruct1>(outValue[i]).f1);
            EXPECT_EQ(std::get<TestStruct1>(value_[i]).f2, std::get<TestStruct1>(outValue[i]).f2);
        } else if (variantType == "TestStruct2") {
            EXPECT_EQ(std::get<TestStruct2>(value_[i]).f1, std::get<TestStruct2>(outValue[i]).f1);
            EXPECT_EQ(std::get<TestStruct2>(value_[i]).f2, std::get<TestStruct2>(outValue[i]).f2);
        }
    }
}

TEST_F(ParamWithVariantArrayTest, FromProto) {
    value_ = VariantArray{};
    VariantArrayParam param(value_, pd_);
    catena::Value protoValue, s1, f1, f2;
    f1.set_int32_value(16);
    f2.set_int32_value(32);
    s1.mutable_struct_variant_value()->set_struct_variant_type("TestStruct1");
    auto fields = s1.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    protoValue.mutable_struct_variant_array_values()->add_struct_variants()->CopyFrom(s1.struct_variant_value());
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(pd_));
    rc_ = param.fromProto(protoValue, authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(alternativeNames<TestVariantStruct>[param.get()[0].index()], s1.struct_variant_value().struct_variant_type());
    EXPECT_EQ(std::get<TestStruct1>(param.get()[0]).f1, f1.int32_value());
    EXPECT_EQ(std::get<TestStruct1>(param.get()[0]).f2, f2.int32_value());
}

TEST_F(ParamWithVariantArrayTest, ValidateSetValue) {
    VariantArrayParam param(value_, pd_);
    catena::Value protoValue, s1, f1, f2;
    f1.set_int32_value(16);
    f2.set_int32_value(32);
    s1.mutable_struct_variant_value()->set_struct_variant_type("TestStruct1");
    auto fields = s1.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    protoValue.mutable_struct_variant_array_values()->add_struct_variants()->CopyFrom(s1.struct_variant_value());
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(pd_));
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

TEST_F(ParamWithVariantArrayTest, ValidateSetValue_SingleElement) {
    VariantArrayParam param(value_, pd_);
    catena::Value protoValue, f1, f2;
    f1.set_int32_value(48);
    f2.set_int32_value(64);
    protoValue.mutable_struct_variant_value()->set_struct_variant_type("TestStruct1");
    auto fields = protoValue.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(pd_));
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

TEST_F(ParamWithVariantArrayTest, ValidateSetValue_Error) {
    VariantArrayParam param(value_, pd_);
    catena::Value protoValue, s1, f1, f2;
    f1.set_int32_value(48);
    f2.set_int32_value(64);
    protoValue.mutable_struct_variant_value()->set_struct_variant_type("TestStruct1");
    auto fields = protoValue.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    for (uint32_t i = 0; i < value_.size() + 1; ++i) {
        protoValue.mutable_struct_variant_array_values()->add_struct_variants()->CopyFrom(s1.struct_variant_value());
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

TEST_F(ParamWithVariantArrayTest, ValidateSetValue_SingleElementError) {
    VariantArrayParam param(value_, pd_);
    catena::Value protoValue, s1, f1, f2;
    f1.set_int32_value(48);
    f2.set_int32_value(64);
    protoValue.mutable_struct_variant_value()->set_struct_variant_type("TestStruct1");
    auto fields = protoValue.mutable_struct_variant_value()->mutable_value()->mutable_struct_value()->mutable_fields();
    fields->insert({"f1", f1});
    fields->insert({"f2", f2});
    EXPECT_CALL(pd_, getSubParam("TestStruct1")).WillRepeatedly(testing::ReturnRef(pd_));
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
