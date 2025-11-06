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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief This file is for testing the EnumDecorator.h file.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-11-06
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// common
#include <patterns/EnumDecorator.h>
#include <Logger.h>
#include <SharedFlags.h>

// gtest
#include <gtest/gtest.h>

// std
#include <cstdint>
#include <string>

using namespace catena::patterns;

/**
 * Test enum for EnumDecorator tests
 */
enum class TestEnum_e : int32_t {
    kZero = 0,
    kOne = 1,
    kTwo = 2,
    kThree = 3
};

// Declare the EnumDecorator
using TestEnum = EnumDecorator<TestEnum_e>;

// Define the forward map
template <>
const TestEnum::FwdMap TestEnum::fwdMap_ = {
    {TestEnum_e::kZero, "zero"},
    {TestEnum_e::kOne, "one"},
    {TestEnum_e::kTwo, "two"},
    {TestEnum_e::kThree, "three"}
};

/**
 * Test fixture class for EnumDecorator tests
 */
class EnumDecoratorTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("EnumDecoratorTest");
    }

    static void TearDownTestSuite() {
    }
};

/*
 * ============================================================================
 *                         Conversion Operator Tests (line 190)
 * ============================================================================
 */

/**
 * TEST 1 - Conversion operator to underlying type (utype) - valid enum value
 */
TEST_F(EnumDecoratorTest, ConversionOperator_ValidValue) {
    TestEnum testEnum(TestEnum_e::kTwo);
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 2);
}

/**
 * TEST 2 - Conversion operator to underlying type - zero value
 */
TEST_F(EnumDecoratorTest, ConversionOperator_ZeroValue) {
    TestEnum testEnum(TestEnum_e::kZero);
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 0);
}

/**
 * TEST 3 - Conversion operator to underlying type - explicit cast
 */
TEST_F(EnumDecoratorTest, ConversionOperator_ExplicitCast) {
    TestEnum testEnum(TestEnum_e::kThree);
    int32_t value = static_cast<int32_t>(testEnum);  // Uses operator utype()
    EXPECT_EQ(value, 3);
}

/**
 * TEST 4 - Conversion operator to underlying type - in arithmetic operations
 */
TEST_F(EnumDecoratorTest, ConversionOperator_InArithmetic) {
    TestEnum testEnum1(TestEnum_e::kOne);
    TestEnum testEnum2(TestEnum_e::kTwo);
    TestEnum::utype sum = testEnum1 + testEnum2;  // Both use operator utype()
    EXPECT_EQ(sum, 3);
}

/**
 * TEST 5 - Conversion operator to underlying type - default constructor
 */
TEST_F(EnumDecoratorTest, ConversionOperator_DefaultConstructor) {
    TestEnum testEnum;  // Default constructor sets to 0
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 0);
}

/**
 * TEST 6 - Conversion operator to underlying type - constructed from string
 */
TEST_F(EnumDecoratorTest, ConversionOperator_FromString) {
    TestEnum testEnum("two");
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 2);
}

/**
 * TEST 7 - Conversion operator to underlying type - constructed from invalid string
 */
TEST_F(EnumDecoratorTest, ConversionOperator_FromInvalidString) {
    TestEnum testEnum("invalid");
    // Invalid string sets value to 0
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 0);
}

/**
 * TEST 8 - Conversion operator to underlying type - constructed from integral value
 */
TEST_F(EnumDecoratorTest, ConversionOperator_FromIntegral) {
    TestEnum testEnum(static_cast<int32_t>(1));
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 1);
}

/**
 * TEST 9 - Conversion operator to underlying type - constructed from invalid integral
 */
TEST_F(EnumDecoratorTest, ConversionOperator_FromInvalidIntegral) {
    TestEnum testEnum(static_cast<int32_t>(99));  // Invalid value, sets to 0
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 0);
}

/*
 * ============================================================================
 *                         Other EnumDecorator Tests
 * ============================================================================
 */

/**
 * TEST 10 - Default constructor
 */
TEST_F(EnumDecoratorTest, DefaultConstructor) {
    TestEnum testEnum;
    EXPECT_EQ(testEnum.value(), TestEnum_e::kZero);
    EXPECT_EQ(testEnum.toString(), "zero");
}

/**
 * TEST 11 - Construct from enum value
 */
TEST_F(EnumDecoratorTest, ConstructFromEnumValue) {
    TestEnum testEnum(TestEnum_e::kTwo);
    EXPECT_EQ(testEnum.value(), TestEnum_e::kTwo);
    EXPECT_EQ(testEnum.toString(), "two");
}

/**
 * TEST 12 - Construct from string
 */
TEST_F(EnumDecoratorTest, ConstructFromString) {
    TestEnum testEnum("one");
    EXPECT_EQ(testEnum.value(), TestEnum_e::kOne);
    EXPECT_EQ(testEnum.toString(), "one");
}

/**
 * TEST 13 - Construct from invalid string
 */
TEST_F(EnumDecoratorTest, ConstructFromInvalidString) {
    TestEnum testEnum("invalid");
    // Invalid string should set value to 0
    EXPECT_EQ(testEnum.value(), TestEnum_e::kZero);
    EXPECT_EQ(testEnum.toString(), "zero");
}

/**
 * TEST 14 - Construct from integral value
 */
TEST_F(EnumDecoratorTest, ConstructFromIntegral) {
    TestEnum testEnum(static_cast<int32_t>(2));
    EXPECT_EQ(testEnum.value(), TestEnum_e::kTwo);
    EXPECT_EQ(testEnum.toString(), "two");
}

/**
 * TEST 15 - Construct from invalid integral value
 */
TEST_F(EnumDecoratorTest, ConstructFromInvalidIntegral) {
    TestEnum testEnum(static_cast<int32_t>(99));
    // Invalid integral should set value to 0
    EXPECT_EQ(testEnum.value(), TestEnum_e::kZero);
    EXPECT_EQ(testEnum.toString(), "zero");
}

/**
 * TEST 16 - Cast to string
 */
TEST_F(EnumDecoratorTest, CastToString) {
    TestEnum testEnum(TestEnum_e::kThree);
    std::string str = testEnum;  // Uses operator std::string()
    EXPECT_EQ(str, "three");
}

/**
 * TEST 17 - toString() method
 */
TEST_F(EnumDecoratorTest, ToString) {
    TestEnum testEnum(TestEnum_e::kOne);
    const std::string& str = testEnum.toString();
    EXPECT_EQ(str, "one");
}

/**
 * TEST 18 - Equality operator
 */
TEST_F(EnumDecoratorTest, EqualityOperator) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(TestEnum_e::kTwo);
    TestEnum testEnum3(TestEnum_e::kThree);
    
    EXPECT_TRUE(testEnum1 == testEnum2);
    EXPECT_FALSE(testEnum1 == testEnum3);
}

/**
 * TEST 19 - Inequality operator
 */
TEST_F(EnumDecoratorTest, InequalityOperator) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(TestEnum_e::kThree);
    TestEnum testEnum3(TestEnum_e::kTwo);
    
    EXPECT_TRUE(testEnum1 != testEnum2);
    EXPECT_FALSE(testEnum1 != testEnum3);
}

/**
 * TEST 20 - Value accessor
 */
TEST_F(EnumDecoratorTest, ValueAccessor) {
    TestEnum testEnum(TestEnum_e::kThree);
    EXPECT_EQ(testEnum.value(), TestEnum_e::kThree);
}

/**
 * TEST 21 - Function call operator
 */
TEST_F(EnumDecoratorTest, FunctionCallOperator) {
    TestEnum testEnum(TestEnum_e::kOne);
    EXPECT_EQ(testEnum(), TestEnum_e::kOne);
}

/**
 * TEST 22 - Copy constructor
 */
TEST_F(EnumDecoratorTest, CopyConstructor) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(testEnum1);
    EXPECT_EQ(testEnum1.value(), testEnum2.value());
    EXPECT_TRUE(testEnum1 == testEnum2);
}

/**
 * TEST 23 - Copy assignment
 */
TEST_F(EnumDecoratorTest, CopyAssignment) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(TestEnum_e::kThree);
    testEnum2 = testEnum1;
    EXPECT_EQ(testEnum1.value(), testEnum2.value());
    EXPECT_TRUE(testEnum1 == testEnum2);
}

/**
 * TEST 24 - Move constructor
 */
TEST_F(EnumDecoratorTest, MoveConstructor) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(std::move(testEnum1));
    EXPECT_EQ(testEnum2.value(), TestEnum_e::kTwo);
}

/**
 * TEST 25 - Move assignment
 */
TEST_F(EnumDecoratorTest, MoveAssignment) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(TestEnum_e::kThree);
    testEnum2 = std::move(testEnum1);
    EXPECT_EQ(testEnum2.value(), TestEnum_e::kTwo);
}

/**
 * TEST 26 - Get forward map
 */
TEST_F(EnumDecoratorTest, GetForwardMap) {
    TestEnum testEnum(TestEnum_e::kOne);
    const TestEnum::FwdMap& fwdMap = testEnum.getForwardMap();
    EXPECT_EQ(fwdMap.size(), 4);
    EXPECT_EQ(fwdMap.at(TestEnum_e::kZero), "zero");
    EXPECT_EQ(fwdMap.at(TestEnum_e::kOne), "one");
    EXPECT_EQ(fwdMap.at(TestEnum_e::kTwo), "two");
    EXPECT_EQ(fwdMap.at(TestEnum_e::kThree), "three");
}

/**
 * TEST 27 - Get reverse map
 */
TEST_F(EnumDecoratorTest, GetReverseMap) {
    TestEnum testEnum(TestEnum_e::kTwo);
    const TestEnum::RevMap& revMap = testEnum.getReverseMap();
    EXPECT_EQ(revMap.size(), 4);
    EXPECT_EQ(revMap.at("zero"), TestEnum_e::kZero);
    EXPECT_EQ(revMap.at("one"), TestEnum_e::kOne);
    EXPECT_EQ(revMap.at("two"), TestEnum_e::kTwo);
    EXPECT_EQ(revMap.at("three"), TestEnum_e::kThree);
}

