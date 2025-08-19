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
 * @brief This file is for testing the ChoiceConstraint.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockDevice.h"

#include "ChoiceConstraint.h"

using namespace catena::common;

/* ============================================================================
 *                                    INT
 * ============================================================================
 * 
 * TEST 1.1 - Testing Int ChoiceConstraint constructors
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_IntCreate) {
    bool shared = false;
    std::string oid = "test_oid";
    { // int32 constructor with no device
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> constraint({{1, {{"en", "one"}}}, {2, {{"en", "two"}}}}, true, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
    { // int32 constructor with device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> constraint({{1, {{"en", "one"}}}, {2, {{"en", "two"}}}}, true, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
}
/* 
 * TEST 1.2 - Testing Int ChoiceConstraint satisfied
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_IntSatisfied) {
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> constraint({{1, {}}, {2, {}}}, true, "test_oid", false);
    catena::Value src;
    // Valid
    src.set_int32_value(1);
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value 1";
    // Valid
    src.set_int32_value(2);
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value 2";
    // Invalid
    src.set_int32_value(3);
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value 3";
}
/* 
 * TEST 1.3 - Testing Int ChoiceConstraint apply
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_IntApply) {
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> constraint({{1, {}}, {2, {}}}, true, "test_oid", false);
    catena::Value src;
    catena::Value res;
    src.set_int32_value(1);
    res = constraint.apply(src);
    EXPECT_EQ(res.SerializeAsString(), "") << "Apply should return an empty value for int32 ChoiceConstraint";
}
/* 
 * TEST 1.4 - Testing Int ChoiceConstraint toProto
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_IntToProto) {
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE>::ListInitializer choicesInit = {
        {1, PolyglotText::ListInitializer{{"en", "one"}}},
        {2, PolyglotText::ListInitializer{{"en", "two"}}}
    };
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE>::Choices choices(choicesInit.begin(), choicesInit.end());
    ChoiceConstraint<int32_t, catena::Constraint::INT_CHOICE> constraint({{1, {{"en", "one"}}}, {2, {{"en", "two"}}}}, true, "test_oid", false);
    catena::Constraint protoConstraint;
    constraint.toProto(protoConstraint);
    // Comparing results
    EXPECT_EQ(protoConstraint.type(), catena::Constraint_ConstraintType_INT_CHOICE);
    EXPECT_EQ(choices.size(), protoConstraint.int32_choice().choices_size());
    for (const auto& protoChoice : protoConstraint.int32_choice().choices()) {
        ASSERT_TRUE(choices.contains(protoChoice.value())) << "Choice value should be in the choices map";
        const auto& protoDS = protoChoice.name().display_strings();
        EXPECT_EQ(choices.at(protoChoice.value()).displayStrings(), PolyglotText::DisplayStrings(protoDS.begin(), protoDS.end()));
    }
}



/* ============================================================================
 *                                  STRING
 * ============================================================================
 * 
 * TEST 2.1 - Testing STRING_CHOICE ChoiceConstraint constructors
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringCreate) {
    bool shared = false;
    std::string oid = "test_oid";
    { // STRING_CHOICE constructor with no device
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, true, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
    { // STRING_CHOICE constructor with device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, true, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
}
/* 
 * TEST 2.2 - Testing STRING_STRING_CHOICE ChoiceConstraint constructors
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringStringCreate) {
    bool shared = false;
    std::string oid = "test_oid";
    { // STRING_STRING_CHOICE constructor with no device
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {{"en", "Choice 1"}}}, {"Choice2", {{"en", "Choice 2"}}}}, true, oid, shared);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
    { // STRING_STRING_CHOICE constructor with device
    MockDevice dm;
    EXPECT_CALL(dm, addItem(oid, testing::An<IConstraint*>())).Times(1).WillOnce(
        testing::Invoke([](const std::string &key, IConstraint *item){
            EXPECT_TRUE(item) << "No item passed into dm.addItem()";
        }));
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {{"en", "Choice 1"}}}, {"Choice2", {{"en", "Choice 2"}}}}, true, oid, shared, dm);
    EXPECT_EQ(constraint.getOid(), oid);
    EXPECT_EQ(constraint.isShared(), shared);
    EXPECT_FALSE(constraint.isRange()) << "ChoiceConstraint should not be a range constraint";
    }
}
/*
 * TEST 2.3 - Testing String ChoiceConstraint satified with strict set to true
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringSatisfiedStrict) {
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, true, "test_oid", false);
    catena::Value src;
    // Valid
    src.set_string_value("Choice1");
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value Choice1";
    // Valid
    src.set_string_value("Choice2");
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value Choice2";
    // Invalid
    src.set_string_value("Choice3");
    EXPECT_FALSE(constraint.satisfied(src)) << "Constraint should not be satisfied by invalid value Choice3";
}
/* 
 * TEST 2.4 - Testing String ChoiceConstraint satified with strict set to false
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringSatisfiedNotStrict) {
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, false, "test_oid", false);
    catena::Value src;
    // Valid
    src.set_string_value("Choice1");
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value Choice1";
    // Valid
    src.set_string_value("Choice2");
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by valid value Choice2";
    // Valid
    src.set_string_value("Choice3");
    EXPECT_TRUE(constraint.satisfied(src)) << "Constraint should be satisfied by invalid value Choice3 if not strict";
}
/* 
 * TEST 2.5 - Testing String ChoiceConstraint apply
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringApply) {
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, true, "test_oid", false);
    catena::Value src;
    catena::Value res;
    src.set_string_value("SomeChoice");
    res = constraint.apply(src);
    EXPECT_EQ(res.SerializeAsString(), "") << "Apply should return an empty value for string ChoiceConstraint";
}
/* 
 * TEST 2.6 - Testing String ChoiceConstraint toProto
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringToProto) {
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE>::ListInitializer choicesInit = {
        {"Choice1", PolyglotText::ListInitializer{}},
        {"Choice2", PolyglotText::ListInitializer{}}
    };
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE>::Choices choices(choicesInit.begin(), choicesInit.end());
    ChoiceConstraint<std::string, catena::Constraint::STRING_CHOICE> constraint({{"Choice1", {}}, {"Choice2", {}}}, true, "test_oid", false);
    catena::Constraint protoConstraint;
    constraint.toProto(protoConstraint);
    // Comparing results
    EXPECT_EQ(protoConstraint.type(), catena::Constraint::STRING_CHOICE);
    EXPECT_EQ(choices.size(), protoConstraint.string_choice().choices_size());
    for (const auto& protoChoice : protoConstraint.string_choice().choices()) {
        EXPECT_TRUE(choices.contains(protoChoice)) << "Choice value should be in the choices map";
    }
}
/* 
 * TEST 2.7 - Testing STRING_STRING_CHOICE ChoiceConstraint toProto
 */
TEST(ChoiceConstraintTest, ChoiceConstraint_StringStringToProto) {
    ChoiceConstraint<std::string, catena::Constraint::STRING_STRING_CHOICE>::ListInitializer choicesInit = {
        {"Choice1", PolyglotText::ListInitializer{{"en", "one"}}},
        {"Choice2", PolyglotText::ListInitializer{{"en", "two"}}}
    };
    ChoiceConstraint<std::string, catena::Constraint::STRING_STRING_CHOICE>::Choices choices(choicesInit.begin(), choicesInit.end());
    ChoiceConstraint<std::string, catena::Constraint::STRING_STRING_CHOICE> constraint({{"Choice1", {{"en", "one"}}}, {"Choice2", {{"en", "two"}}}}, true, "test_oid", false);
    catena::Constraint protoConstraint;
    constraint.toProto(protoConstraint);
    // Comparing results
    EXPECT_EQ(protoConstraint.type(), catena::Constraint::STRING_STRING_CHOICE);
    EXPECT_EQ(choices.size(), protoConstraint.string_string_choice().choices_size());
    for (const auto& protoChoice : protoConstraint.string_string_choice().choices()) {
        ASSERT_TRUE(choices.contains(protoChoice.value())) << "Choice value should be in the choices map";
        const auto& protoDS = protoChoice.name().display_strings();
        EXPECT_EQ(choices.at(protoChoice.value()).displayStrings(), PolyglotText::DisplayStrings(protoDS.begin(), protoDS.end()));
    }
}
