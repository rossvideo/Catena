/*
 * Copyright 2026 Ross Video Ltd
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
 * @brief This file is for testing the RangeConstraint.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @author (Nelson Daniels) nelson.daniels@rossvideo.com
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-02-19
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockDevice.h"
#include "Config.h"
#include "Logger.h"

#include "RangeConstraint.h"

using namespace catena::common;

// Test fixture class for RangeConstraint tests
class RangeConstraintTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        config::log_dir = UNITTEST_LOG_DIR;
        config::log_file = true;
        config::log_level = "INFO";
        config::log_size = 10;
        config::log_count = 128;
        config::log_verbosity = 2;
        Logger::init("RangeConstraintTest");
    }

    static void TearDownTestSuite() {
    }
};

/* 
 * TEST 0.1 - Testing RangeConstraint constructor when the type is not int or float
 */
TEST_F(RangeConstraintTest, RangeConstraint_InvalidCreate) {
    { // Without device
    EXPECT_THROW(RangeConstraint<std::string> constraint("min", "max", "step", "test_oid", false), std::runtime_error)
        << "Constructor should throw an error when defined with a type other than int and float";
    }
    { // With device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(::testing::_, testing::An<IConstraint*>())).Times(0);
    EXPECT_THROW(RangeConstraint<std::string> constraint("min", "max", "step", "test_oid", false, dm), std::runtime_error)
        << "Constructor should throw an error when defined with a type other than int and float";
    }
    { // With display min and display max
    EXPECT_THROW(RangeConstraint<std::string> constraint("min", "max", "step", "display_min", "display_max", "test_oid", false), std::runtime_error)
        << "Constructor should throw an error when defined with a type other than int and float";
    }
    { // With display min, display max, and device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(::testing::_, testing::An<IConstraint*>())).Times(0);
    EXPECT_THROW(RangeConstraint<std::string> constraint("min", "max", "step", "display_min", "display_max", "test_oid", false, dm), std::runtime_error)
        << "Constructor should throw an error when defined with a type other than int and float";
    }
}



/* ============================================================================
 *                                    INT
 * ============================================================================
 * 
 * TEST 1.1 - Testing Int RangeConstraint constructors
 */
TEST_F(RangeConstraintTest, RangeConstraint_IntCreate) {
    bool shared = false;
    std::string oid = "test_oid";
    { // Without device
    RangeConstraint<int32_t> constraint(0, 10, 2, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    RangeConstraint<int32_t> constraint(0, 10, 2, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With display min and display max
    RangeConstraint<int32_t> constraint(0, 10, 2, 2, 8, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With display min, display max, and device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    RangeConstraint<int32_t> constraint(0, 10, 2, 2, 8, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
}
/* 
 * TEST 1.2 - Testing Int RangeConstraint satisfied
 */
TEST_F(RangeConstraintTest, RangeConstraint_IntSatisfied) {
    RangeConstraint<int32_t> constraint(0, 10, 2, "test_oid", false);
    st2138::Value src;
    // Valid
    src.set_int32_value(4);
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value 4";
    // Invalid
    src.set_int32_value(-1);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value -2";
    // Invalid
    src.set_int32_value(11);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 12";
    // Invalid
    src.set_int32_value(3);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 3 with step 2";
}
/* 
 * TEST 1.3 - Testing Int RangeConstraint apply
 */
TEST_F(RangeConstraintTest, RangeConstraint_IntApply) {
    int32_t min = 0, max = 10, step = 2;
    RangeConstraint<int32_t> constraint(min, max, step, "test_oid", false);
    st2138::Value src;
    { // Valid - do nothing
    src.set_int32_value(4);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.int32_value(), src.int32_value()) << "Constraint should not change valid value 4";
    }
    { // Invalid - < min
    src.set_int32_value(-2);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.int32_value(), min) << "Constraint should set invalid value -2 to min 0";
    }
    { // Invalid - > max
    src.set_int32_value(12);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.int32_value(), max) << "Constraint should set invalid value 12 to max 10";
    }
    { // Invalid - % step != 0
    src.set_int32_value(3);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.int32_value(), 2) << "Constraint should set invalid value 3 to 2";
    }
}
/* 
 * TEST 1.4 - Testing Int RangeConstraint toProto
 */
TEST_F(RangeConstraintTest, RangeConstraint_IntToProto) {
    int32_t min = 0, max = 10, step = 2, displayMin = 2, displayMax = 8;
    RangeConstraint<int32_t> constraint(min, max, step, displayMin, displayMax, "test_oid", false);
    st2138::Constraint protoConstraint;
    constraint.toProto(protoConstraint);
    // Comparing results
    EXPECT_EQ(protoConstraint.type(), st2138::Constraint::INT_RANGE);
    EXPECT_EQ(protoConstraint.int32_range().min_value(), min);
    EXPECT_EQ(protoConstraint.int32_range().max_value(), max);
    EXPECT_EQ(protoConstraint.int32_range().step(), step);
    EXPECT_EQ(protoConstraint.int32_range().display_min(), displayMin);
    EXPECT_EQ(protoConstraint.int32_range().display_max(), displayMax);
}



/* ============================================================================
 *                                   FLOAT
 * ============================================================================
 * 
 * TEST 2.1 - Testing FLOAT RangeConstraint constructors
 */
TEST_F(RangeConstraintTest, RangeConstraint_FloatCreate) {
    bool shared = false;
    std::string oid = "test_oid";
    { // Without device
    RangeConstraint<float> constraint(0, 10, 2, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    RangeConstraint<float> constraint(0, 10, 2, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With display min and display max
    RangeConstraint<float> constraint(0, 10, 2, 2, 8, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
    { // With display min, display max, and device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    RangeConstraint<float> constraint(0, 10, 2, 2, 8, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_TRUE(constraint.isRange()) << "RangeConstraint should be a range constraint";
    }
}
/* 
 * TEST 2.2 - Testing Float RangeConstraint satisfied
 */
TEST_F(RangeConstraintTest, RangeConstraint_FloatSatisfied) {
    RangeConstraint<float> constraint(0.5, 9.5, 0.5, "test_oid", false);
    st2138::Value src;
    // Valid
    src.set_float32_value(4.5);
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value 4.5";
    // Invalid
    src.set_float32_value(0);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 0";
    // Invalid
    src.set_float32_value(10);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 10";
    // Invalid
    src.set_float32_value(3.25);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 3.25 with step 0.5";
    constraint = RangeConstraint<float>(-10, 10, 0.1, "test_oid", false);
    for (int testInt = -1000; testInt <= 1000; testInt += 10) {
        // valid right on the step
        float testVal = testInt / 100.0f;
        src.set_float32_value(testVal);
        EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value " << testVal;
        // test the invalid values in between the steps
        for (int between = 1; between <= 9; between += 1) {
            testVal = ((testInt + between) / 100.0f) ;
            src.set_float32_value(testVal);
            EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value " << testVal;
        }
    }
}

/* 
 * TEST 2.3 - Testing Float RangeConstraint apply
 */
TEST_F(RangeConstraintTest, RangeConstraint_FloatApply) {
    float min = 0.5, max = 9.5, step = 0.5;
    RangeConstraint<float> constraint(min, max, step, "test_oid", false);
    st2138::Value src;
    { // Valid - do nothing
    src.set_float32_value(4.5);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), src.float32_value()) << "Constraint should not change valid value 4.5";
    }
    { // Invalid - < min
    src.set_float32_value(0);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), min) << "Constraint should set invalid value 0 to min 0.5";
    }
    { // Invalid - > max
    src.set_float32_value(10);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), max) << "Constraint should set invalid value 10 to max 9.5";
    }
    { // Invalid - % step != 0
    src.set_float32_value(3.25);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), 3.5) << "Constraint should set invalid value 3.25 to 3.5";
    }
    // some more testing for floating point precision issues
    min = -10; max = 10; step = 0.1;
    constraint = RangeConstraint<float>(min, max, step, "test_oid", false);
    for (int expected = -1000; expected <= 1000; expected += 10) {
        const float testVal = expected / 100.0f;
        src.set_float32_value(testVal);
        st2138::Value res = constraint.apply(src);
        // just care about the value being close enough to the expected value due to floating point precision issues
        EXPECT_NEAR(res.float32_value(), testVal, 0.0001f) << "Constraint should not change valid value " << testVal;
    }
    // testing in between values
    // -9.99 -> -10.0, -9.98 -> -10.0, -9.94 -> -9.9, -9.91 -> -9.9 ...,  9.96 -> 10.0
    for (int expected = -1000; expected <= 1000; expected += 10) {
        for (int between = -4; between <= 4; between += 1) {
            // shift decimal place to cause precision issues
            const float testVal = ((expected + between) / 100.0f) ;
            src.set_float32_value(testVal);
            st2138::Value res = constraint.apply(src);
            EXPECT_NEAR(res.float32_value(), expected / 100.0f, 0.0001f) << "Constraint should set invalid value " << testVal << " to " << expected / 100.0f;
        }
    }
    // "exactly" in between is undefined, so we'll check either up or down
    for (int expected = -1000; expected < 1000; expected += 10) {
        const float testVal = (expected + 5) / 100.0f;
        src.set_float32_value(testVal);
        st2138::Value res = constraint.apply(src);
        const float resVal = res.float32_value();
        EXPECT_TRUE(std::abs(resVal - (expected / 100.0f)) < 0.0001f || std::abs(resVal - ((expected + 10) / 100.0f)) < 0.0001f) << "Constraint should set invalid value " << testVal << " to either " << expected / 100.0f << " or " << (expected + 10) / 100.0f;
    }
    min = -10; max = 10; step = 7;
    constraint = RangeConstraint<float>(min, max, step, "test_oid", false);
    // make sure apply doesn't go under the min when rounding to the nearest step
    src.set_float32_value(-9);
    st2138::Value res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), -10) << "Constraint should set invalid value -9 to -10";
    // make sure apply doesn't go over the max when rounding to the nearest step
    // naive rounding would round 9 to 11 which is over the max
    src.set_float32_value(9);
    res = constraint.apply(src);
    EXPECT_EQ(res.float32_value(), 10) << "Constraint should set invalid value 9 to 10";
}
/* 
 * TEST 2.4 - Testing Float RangeConstraint toProto
 */
TEST_F(RangeConstraintTest, RangeConstraint_FloatToProto) {
    float min = 0.5, max = 9.5, step = 0.5, displayMin = 2, displayMax = 8;
    RangeConstraint<float> constraint(min, max, step, displayMin, displayMax, "test_oid", false);
    st2138::Constraint protoConstraint;
    constraint.toProto(protoConstraint);
    // Comparing results
    EXPECT_EQ(protoConstraint.type(), st2138::Constraint::FLOAT_RANGE);
    EXPECT_EQ(protoConstraint.float_range().min_value(), min);
    EXPECT_EQ(protoConstraint.float_range().max_value(), max);
    EXPECT_EQ(protoConstraint.float_range().step(), step);
    EXPECT_EQ(protoConstraint.float_range().display_min(), displayMin);
    EXPECT_EQ(protoConstraint.float_range().display_max(), displayMax);
}
