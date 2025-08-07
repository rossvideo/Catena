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
 * @brief This file is for testing the <std::vector<std::string>>ParamWithValue
 * class.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/31
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "Param_test.h"

using namespace catena::common;

using StringArray = std::vector<std::string>;
using StringArrayParam = ParamWithValue<StringArray>;

class ParamWithStringArrayTest : public ParamTest<StringArray> {
  protected:
    catena::ParamType type() const override { return catena::ParamType::STRING_ARRAY; }

    StringArray value_{"Hello", "World"};
};

/**
 * TEST 1 - Testing <StringArray>ParamWithValue constructors
 */
TEST_F(ParamWithStringArrayTest, Create) {
    CreateTest(value_);
}

/**
 * TEST 2 - Testing <StringArray>ParamWithValue.get()
 */
TEST_F(ParamWithStringArrayTest, Get) {
    GetValueTest(value_);
}

/**
 * TEST 3 - Testing <StringArray>ParamWithValue.size()
 */
TEST_F(ParamWithStringArrayTest, Size) {
    StringArrayParam param(value_, pd_);
    EXPECT_EQ(param.size(), value_.size());
}

/**
 * TEST 4 - Testing <StringArray>ParamWithValue.getParam()
 */
TEST_F(ParamWithStringArrayTest, GetParam) {
    StringArrayParam param(value_, pd_);
    Path path = Path("/0");
    auto foundParam = param.getParam(path, authz_, rc_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    ASSERT_TRUE(foundParam) << "Did not find a parameter when one was expected";
    EXPECT_EQ(getParamValue<std::string>(foundParam.get()), value_[0]);
}

/**
 * TEST 5 - Testing <StringArray>ParamWithValue.getParam() error handling.
 * 
 * Four main error cases:
 *  - Front is not an index
 *  - Index is out of bounds
 *  - Param does not exist
 *  - Not authorized
 */
TEST_F(ParamWithStringArrayTest, GetParam_Error) {
    StringArrayParam param(value_, pd_);
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
/**
 * TEST 6 - Testing <StringArray>ParamWithValue.addBack()
 */
TEST_F(ParamWithStringArrayTest, AddBack) {
    StringArrayParam param(value_, pd_);
    auto addedParam = param.addBack(authz_, rc_);
    EXPECT_TRUE(addedParam) << "Failed to add a value to array parameter";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}
/**
 * TEST 7 - Testing <StringArray>ParamWithValue.addBack() error handling.
 * 
 * Two main error cases:
 *  - Array is at max length
 *  - Not authorized
 */
TEST_F(ParamWithStringArrayTest, AddBack_Error) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    { // Add exceeds max length
    EXPECT_CALL(pd_, max_length()).WillOnce(testing::Return(2));
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
/**
 * TEST 8 - Testing <StringArray>ParamWithValue.popBack().
 */
TEST_F(ParamWithStringArrayTest, PopBack) {
    StringArrayParam param(value_, pd_);
    StringArray valueCopy{value_.begin(), value_.end()};
    rc_ = param.popBack(authz_);
    valueCopy.pop_back();

    EXPECT_EQ(param.get(), valueCopy);
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
}

/**
 * TEST 9 - Testing <StringArray>ParamWithValue.popBack() error handling.
 * 
 * Two main error cases:
 *  - Array is empty
 *  - Not authorized
 */
TEST_F(ParamWithStringArrayTest, PopBack_Error) {
    std::vector<std::string> value{};
    StringArrayParam param(value, pd_);

    // Empty array
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "popBack should return OUT_OF_RANGE if array is empty";
    
    // Not authorized
    EXPECT_CALL(authz_, writeAuthz(testing::Matcher<const IParamDescriptor&>(testing::Ref(pd_)))).WillOnce(testing::Return(false));
    rc_ = param.popBack(authz_);
    EXPECT_EQ(rc_.status, catena::StatusCode::PERMISSION_DENIED)
        << "popBack should return PERMISSION_DENNIED if Authorizer does not have writeAuthz";
}

/**
 * TEST 10 - Testing <StringArray>ParamWithValue.toProto(catena::Param)
 */
TEST_F(ParamWithStringArrayTest, ToProto) {
    StringArrayParam param(value_, pd_);
    std::vector<std::string> outValue{};
    catena::Param outParam;
    EXPECT_CALL(pd_, toProto(testing::An<catena::Param&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Param& p, const IAuthorizer&) {
            p.set_template_oid(oid_);
        }));
    rc_ = param.toProto(outParam, authz_);
    ASSERT_TRUE(outParam.value().has_string_array_values());
    ASSERT_EQ(fromProto(outParam.value(), &outValue, pd_, authz_).status, catena::StatusCode::OK) << "fromProto failed, cannot compare results.";
    EXPECT_EQ(rc_.status, catena::StatusCode::OK);
    EXPECT_EQ(value_, outValue);
    EXPECT_EQ(oid_, outParam.template_oid());
}

/**
 * TEST 11 - Testing <StringArray>ParamWithValue.validateSetValue()
 */
TEST_F(ParamWithStringArrayTest, ValidateSetValue) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    for (std::string i : {"Hello", "World", "!"}) {
        protoValue.mutable_string_array_values()->add_strings(i);
    }
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_)) << "Valid setting whole array";
}

/**
 * TEST 11 - Testing <StringArray>ParamWithValue.validateSetValue() for
 * appending and setting a single element.
 */
TEST_F(ParamWithStringArrayTest, ValidateSetValue_SingleElement) {
    std::vector<std::string> value{"Hello", "World"};
    StringArrayParam param(value, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("Goodbye");
    // Setting existing value.
    EXPECT_TRUE(param.validateSetValue(protoValue, 0, authz_, rc_)) << "Valid set existing value";
    // Appending to the end.
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_)) << "Valid append value";
}

/**
 * TEST 12 - Testing <StringArray>ParamWithValue.validateSetValue() error handling.
 * 
 * Three main error cases:
 *  - Index is defined
 *  - New value exceeds maxLength
 *  - New value exceeds totalLength
 */
TEST_F(ParamWithStringArrayTest, ValidateSetValue_Error) {
    StringArrayParam param(value_, pd_);
    catena::Value protoValue;
    for (std::string i : {"Hello", "World", "Goodbye"}) {
        protoValue.mutable_string_array_values()->add_strings(i);
    }

    // Index is defined
    EXPECT_FALSE(param.validateSetValue(protoValue, 1, authz_, rc_))
        << "Should return false when index is defined for non-element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when index is defined for non-element setValue";

    // New value exceeds maxLength
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(value_.size()));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds maxLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds maxLength";
    
    // New value exceeds totalLength
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(1000));
    EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(10));
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the new value exceeds totalLength";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the new value exceeds totalLength";
}

/**
 * TEST 12 - Testing <StringArray>ParamWithValue.validateSetValue() error handling
 * when appending and setting a single element.
 * 
 * Three main error cases:
 *  - Index is undefined
 *  - Index is out of bounds
 *  - New value exceeds maxLength
 *  - New value exceeds totalLength
 */
TEST_F(ParamWithStringArrayTest, ValidateSetValue_SingleElementError) {
    StringArrayParam param(value_, pd_);
    catena::Value protoValue;
    protoValue.set_string_value("!");
    EXPECT_CALL(pd_, max_length()).WillRepeatedly(testing::Return(5));

    // Undefined index
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kNone, authz_, rc_))
        << "Should return false when the index is undefined for single element setValue";
    EXPECT_EQ(rc_.status, catena::StatusCode::INVALID_ARGUMENT)
        << "Should return INVALID_ARGUMENT when the index is undefined for single element setValue";

    // Index out of bounds
    EXPECT_FALSE(param.validateSetValue(protoValue, value_.size(), authz_, rc_))
        << "Should return false when the index is out of bounds of the array";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the index is out of bounds of the array";

    // Exceeds max_length
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_TRUE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "value should be able to append at 3 elements";
    EXPECT_FALSE(param.validateSetValue(protoValue, Path::kEnd, authz_, rc_))
        << "Should return false when the array length exceeds max_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds max_length";
    
    // Exceeds total_length
    param.resetValidate(); // Need to reset the trackers so max_length does not trigger.
    protoValue.set_string_value("This is a long string");
    EXPECT_CALL(pd_, total_length()).WillRepeatedly(testing::Return(15));
    EXPECT_FALSE(param.validateSetValue(protoValue, 0, authz_, rc_))
        << "Should return false when the array length exceeds total_length";
    EXPECT_EQ(rc_.status, catena::StatusCode::OUT_OF_RANGE)
        << "Should return OUT_OF_RANGE when the array length exceeds total_length";
}
