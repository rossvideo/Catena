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
    kThree = 3,
    kFour = 4  // Not in forward map - used to test lines 199, 210, 211
};

// Declare the EnumDecorator
using TestEnum = EnumDecorator<TestEnum_e>;

// Define the forward map (kFour is intentionally omitted)
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
 * TEST - Conversion operator to underlying type (line 190)
 */
TEST_F(EnumDecoratorTest, ConversionOperator) {
    TestEnum testEnum(TestEnum_e::kTwo);
    TestEnum::utype underlyingValue = testEnum;  // Uses operator utype()
    EXPECT_EQ(underlyingValue, 2);
    
    // Test with arithmetic operations
    TestEnum testEnum2(TestEnum_e::kOne);
    TestEnum::utype sum = testEnum + testEnum2;
    EXPECT_EQ(sum, 3);
}

/*
 * ============================================================================
 *                         String Conversion Tests (lines 199, 210, 211)
 * ============================================================================
 */

/**
 * TEST - Cast to string when value is NOT in map (line 199)
 */
TEST_F(EnumDecoratorTest, CastToString_ValueNotInMap) {
    // kFour is not in the forward map
    TestEnum testEnum(TestEnum_e::kFour);
    std::string str = testEnum;  // Uses operator std::string() - line 199
    EXPECT_TRUE(str.empty());
}

/**
 * TEST - toString() when value is NOT in map (lines 210, 211)
 */
TEST_F(EnumDecoratorTest, ToString_ValueNotInMap) {
    // kFour is not in the forward map
    TestEnum testEnum(TestEnum_e::kFour);
    const std::string& str = testEnum.toString();  // Lines 210, 211
    EXPECT_TRUE(str.empty());
}

/**
 * TEST - String conversion when value IS in map
 */
TEST_F(EnumDecoratorTest, StringConversion_ValueInMap) {
    TestEnum testEnum(TestEnum_e::kTwo);
    std::string str = testEnum;  // Uses operator std::string()
    EXPECT_EQ(str, "two");
    
    const std::string& strRef = testEnum.toString();
    EXPECT_EQ(strRef, "two");
}

/*
 * ============================================================================
 *                         Constructor Tests
 * ============================================================================
 */

/**
 * TEST - Default constructor
 */
TEST_F(EnumDecoratorTest, DefaultConstructor) {
    TestEnum testEnum;
    EXPECT_EQ(testEnum.value(), TestEnum_e::kZero);
    EXPECT_EQ(testEnum.toString(), "zero");
}

/**
 * TEST - Construct from enum value
 */
TEST_F(EnumDecoratorTest, ConstructFromEnumValue) {
    TestEnum testEnum(TestEnum_e::kTwo);
    EXPECT_EQ(testEnum.value(), TestEnum_e::kTwo);
    EXPECT_EQ(testEnum.toString(), "two");
}

/**
 * TEST - Construct from string
 */
TEST_F(EnumDecoratorTest, ConstructFromString) {
    TestEnum testEnum("one");
    EXPECT_EQ(testEnum.value(), TestEnum_e::kOne);
    EXPECT_EQ(testEnum.toString(), "one");
    
    // Invalid string sets to 0
    TestEnum testEnum2("invalid");
    EXPECT_EQ(testEnum2.value(), TestEnum_e::kZero);
}

/**
 * TEST - Construct from integral value
 */
TEST_F(EnumDecoratorTest, ConstructFromIntegral) {
    TestEnum testEnum(static_cast<int32_t>(2));
    EXPECT_EQ(testEnum.value(), TestEnum_e::kTwo);
    
    // Invalid integral sets to 0
    TestEnum testEnum2(static_cast<int32_t>(99));
    EXPECT_EQ(testEnum2.value(), TestEnum_e::kZero);
}

/*
 * ============================================================================
 *                         Operator Tests
 * ============================================================================
 */

/**
 * TEST - Equality and inequality operators
 */
TEST_F(EnumDecoratorTest, EqualityOperators) {
    TestEnum testEnum1(TestEnum_e::kTwo);
    TestEnum testEnum2(TestEnum_e::kTwo);
    TestEnum testEnum3(TestEnum_e::kThree);
    
    EXPECT_TRUE(testEnum1 == testEnum2);
    EXPECT_FALSE(testEnum1 == testEnum3);
    EXPECT_TRUE(testEnum1 != testEnum3);
    EXPECT_FALSE(testEnum1 != testEnum2);
}

/**
 * TEST - Value accessors
 */
TEST_F(EnumDecoratorTest, ValueAccessors) {
    TestEnum testEnum(TestEnum_e::kThree);
    EXPECT_EQ(testEnum.value(), TestEnum_e::kThree);
    EXPECT_EQ(testEnum(), TestEnum_e::kThree);
}

/*
 * ============================================================================
 *                         Map Access Tests
 * ============================================================================
 */

/**
 * TEST - Get forward and reverse maps
 */
TEST_F(EnumDecoratorTest, MapAccess) {
    TestEnum testEnum(TestEnum_e::kOne);
    const TestEnum::FwdMap& fwdMap = testEnum.getForwardMap();
    EXPECT_EQ(fwdMap.size(), 4);
    EXPECT_EQ(fwdMap.at(TestEnum_e::kTwo), "two");
    
    const TestEnum::RevMap& revMap = testEnum.getReverseMap();
    EXPECT_EQ(revMap.size(), 4);
    EXPECT_EQ(revMap.at("two"), TestEnum_e::kTwo);
}

