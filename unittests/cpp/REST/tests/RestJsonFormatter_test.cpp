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
 * @brief Isolated tests for the REST JSON post-processor (JsonFixers +
 * RestJsonFormatter). This is the only place the fixer code is exercised; all
 * other REST suites bypass it (see ScopedFormatterBypass in RESTTest.h) so any
 * coverage here reflects genuinely tested behaviour.
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>

// REST
#include <JsonFixers.h>
#include <RestJsonFormatter.h>

// picojson
#include <picojson/picojson.h>

// proto
#include <interface/device.pb.h>

// common
#include <Status.h>

// std
#include <string>

using catena::REST::RestJsonFormatter;
using catena::REST::fixDevice;
using catena::REST::fixPushUpdates;
using catena::REST::injectIfAbsent;
using catena::REST::stripEmptyValues;
using catena::REST::stripResponseFalseUnderParams;
using catena::REST::stripZeroFields;

namespace {

// Parses a JSON string and returns the picojson value, failing the test on
// parse error.
picojson::value parse(const std::string& json) {
    picojson::value v;
    const std::string err = picojson::parse(v, json);
    EXPECT_TRUE(err.empty()) << "parse error: " << err << " for: " << json;
    return v;
}

// Round-trips a JSON string through a primitive operating on a picojson tree.
// picojson serializes object keys in sorted (std::map) order, so expectations
// are deterministic.
std::string applyStrip(const std::string& json, void (*fn)(picojson::value&)) {
    picojson::value v = parse(json);
    fn(v);
    return v.serialize();
}

}  // namespace

// ============================================================================
//                              Primitive tests
// ============================================================================

TEST(JsonFixers, StripEmptyValues_RemovesEmptiesAndCascades) {
    picojson::value v = parse(
        R"({"a":"","b":null,"c":{},"d":[],"e":{"f":""},"g":"keep","h":{"i":"x"}})");
    EXPECT_FALSE(stripEmptyValues(v));
    // "e" cascades away because its only member "f" is an empty string.
    EXPECT_EQ(v.serialize(), R"({"g":"keep","h":{"i":"x"}})");
}

TEST(JsonFixers, StripEmptyValues_TopLevelEmptyReportsEmpty) {
    picojson::value v = parse(R"({"a":{},"b":[]})");
    EXPECT_TRUE(stripEmptyValues(v));
    EXPECT_EQ(v.serialize(), R"({})");
}

TEST(JsonFixers, StripEmptyValues_KeepsArrayElementsButCleansThem) {
    // Array elements are cleaned in place but never removed (index stability).
    picojson::value v = parse(R"({"arr":[{"x":""},{"y":"keep"}]})");
    EXPECT_FALSE(stripEmptyValues(v));
    EXPECT_EQ(v.serialize(), R"({"arr":[{},{"y":"keep"}]})");
}

TEST(JsonFixers, StripEmptyValues_NonObjectOrArrayIsNoOp) {
    picojson::value v = parse(R"("string")");
    EXPECT_FALSE(stripEmptyValues(v));
    EXPECT_EQ(v.serialize(), R"("string")");
}

TEST(JsonFixers, StripZeroFields_RemovesNamedZerosOnly) {
    picojson::value v = parse(
        R"({"precision":0,"max_length":0,"total_length":5,"value":0,"nested":{"precision":0,"keep":1}})");
    stripZeroFields(v, {"precision", "max_length", "total_length"});
    // total_length kept (non-zero), value kept (not listed), nested.precision removed.
    EXPECT_EQ(v.serialize(), R"({"nested":{"keep":1},"total_length":5,"value":0})");
}

TEST(JsonFixers, StripZeroFields_RecursesIntoArrays) {
    picojson::value v = parse(R"({"items":[{"precision":0,"keep":2}]})");
    stripZeroFields(v, {"precision"});
    EXPECT_EQ(v.serialize(), R"({"items":[{"keep":2}]})");
}

TEST(JsonFixers, StripResponseFalseUnderParams_OnlyAffectsParams) {
    picojson::value v = parse(
        R"({"commands":{"c1":{"response":false}},"params":{"p1":{"response":false,"x":1},"p2":{"response":true}}})");
    stripResponseFalseUnderParams(v);
    // response:false removed under params only; response:true preserved;
    // commands untouched.
    EXPECT_EQ(v.serialize(),
              R"({"commands":{"c1":{"response":false}},"params":{"p1":{"x":1},"p2":{"response":true}}})");
}

TEST(JsonFixers, StripResponseFalseUnderParams_NoParamsIsNoOp) {
    picojson::value v = parse(R"({"commands":{"c1":{"response":false}}})");
    stripResponseFalseUnderParams(v);
    EXPECT_EQ(v.serialize(), R"({"commands":{"c1":{"response":false}}})");
}

TEST(JsonFixers, InjectIfAbsent_AddsWhenMissing) {
    picojson::value v = parse(R"({"a":1})");
    injectIfAbsent(v, "slot", picojson::value(static_cast<double>(0)));
    EXPECT_EQ(v.serialize(), R"({"a":1,"slot":0})");
}

TEST(JsonFixers, InjectIfAbsent_NoOpWhenPresent) {
    picojson::value v = parse(R"({"a":1,"slot":7})");
    injectIfAbsent(v, "slot", picojson::value(static_cast<double>(0)));
    EXPECT_EQ(v.serialize(), R"({"a":1,"slot":7})");
}

// ============================================================================
//                          Composition (PostFn) tests
// ============================================================================

TEST(JsonFixers, FixDevice_StripsZerosResponseAndEmpties) {
    // Simulates emit-unpopulated output: an empty param collapses entirely
    // while a param with real content survives; slot:0 is preserved.
    std::string json = R"({)"
                       R"("slot":0,)"
                       R"("params":{)"
                       R"("count":{"precision":0,"max_length":0,"total_length":0,"response":false,"params":{},"value":{}},)"
                       R"("label":{"value":{"string_value":"hi"}})"
                       R"(},)"
                       R"("empty_map":{})"
                       R"(})";
    st2138::Device unused;  // msg argument is unused by fixDevice
    fixDevice(unused, json);
    EXPECT_EQ(json, R"({"params":{"label":{"value":{"string_value":"hi"}}},"slot":0})");
}

TEST(JsonFixers, FixPushUpdates_InjectsSlotZeroWhenAbsent) {
    st2138::PushUpdates pu;
    pu.set_slot(0);
    std::string json = R"({"value":{"oid":"x"}})";
    fixPushUpdates(pu, json);
    EXPECT_EQ(json, R"({"slot":0,"value":{"oid":"x"}})");
}

TEST(JsonFixers, FixPushUpdates_KeepsExistingSlot) {
    st2138::PushUpdates pu;
    pu.set_slot(7);
    std::string json = R"({"slot":7,"value":{"oid":"x"}})";
    fixPushUpdates(pu, json);
    EXPECT_EQ(json, R"({"slot":7,"value":{"oid":"x"}})");
}

TEST(JsonFixers, FixPushUpdates_IgnoresNonPushUpdatesMessage) {
    st2138::Value notPush;
    notPush.set_string_value("hi");
    std::string json = R"({"string_value":"hi"})";
    fixPushUpdates(notPush, json);
    EXPECT_EQ(json, R"({"string_value":"hi"})");
}

// ============================================================================
//                          format() dispatch tests
// ============================================================================

// Restores the formatter's rule table after any test that mutates it.
class FormatterTest : public ::testing::Test {
  protected:
    void SetUp() override { saved_ = RestJsonFormatter::getInstance().snapshot(); }
    void TearDown() override { RestJsonFormatter::getInstance().restore(std::move(saved_)); }
    RestJsonFormatter::RuleTable saved_;
};

TEST_F(FormatterTest, Format_UnregisteredTypeIsSparseWithProtoNames) {
    st2138::Value v;
    v.set_string_value("hi");
    std::string out;
    auto rc = RestJsonFormatter::getInstance().format(v, out);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    // No rule -> sparse output; preserve_proto_field_names keeps snake_case.
    EXPECT_EQ(out, R"({"string_value":"hi"})");
}

TEST_F(FormatterTest, Format_PushUpdatesAlwaysHasSlot) {
    st2138::PushUpdates pu;
    pu.set_slot(0);
    std::string out;
    auto rc = RestJsonFormatter::getInstance().format(pu, out);
    ASSERT_EQ(rc.status, catena::StatusCode::OK);
    picojson::value parsed = parse(out);
    ASSERT_TRUE(parsed.is<picojson::object>());
    ASSERT_TRUE(parsed.contains("slot"));
    EXPECT_EQ(parsed.get("slot").get<double>(), 0.0);
}

TEST_F(FormatterTest, Format_DeviceKeepsSlotZeroAndDropsEmpties) {
    st2138::Device dev;
    dev.set_slot(0);
    std::string out;
    auto rc = RestJsonFormatter::getInstance().format(dev, out);
    ASSERT_EQ(rc.status, catena::StatusCode::OK);
    picojson::value parsed = parse(out);
    ASSERT_TRUE(parsed.is<picojson::object>());
    // slot:0 is emitted (no-presence) and survives the empty-strip.
    ASSERT_TRUE(parsed.contains("slot"));
    EXPECT_EQ(parsed.get("slot").get<double>(), 0.0);
    // Empty maps/collections are not present.
    EXPECT_FALSE(parsed.contains("params"));
    EXPECT_FALSE(parsed.contains("commands"));
}

TEST_F(FormatterTest, Format_RespectsTestInstalledRule) {
    // Replace the table with a single rule for Value that injects a marker.
    RestJsonFormatter::Rule rule;
    rule.post = [](const google::protobuf::Message&, std::string& json) {
        picojson::value v;
        if (picojson::parse(v, json).empty() && v.is<picojson::object>()) {
            v.get<picojson::object>()["marker"] = picojson::value(true);
            json = v.serialize();
        }
    };
    RestJsonFormatter::getInstance().restore({});
    RestJsonFormatter::getInstance().setRule(st2138::Value::descriptor(), std::move(rule));
    EXPECT_TRUE(RestJsonFormatter::getInstance().hasRule(st2138::Value::descriptor()));

    st2138::Value v;
    v.set_string_value("hi");
    std::string out;
    auto rc = RestJsonFormatter::getInstance().format(v, out);
    ASSERT_EQ(rc.status, catena::StatusCode::OK);
    EXPECT_EQ(out, R"({"marker":true,"string_value":"hi"})");
}

TEST_F(FormatterTest, Format_NoRuleAfterRestoreEmpty) {
    RestJsonFormatter::getInstance().restore({});
    EXPECT_FALSE(RestJsonFormatter::getInstance().hasRule(st2138::Device::descriptor()));
}
