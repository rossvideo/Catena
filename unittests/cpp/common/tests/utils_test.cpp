/**
* @file utils_tests.cpp
* @brief Testing for utils.cpp
* @author nathan.rochon@rossvideo.com
* @date 2025-03-26
* @copyright Copyright © 2025 Ross Video Ltd
*/

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <Logger.h>
#include "../src/utils.cpp" // Include the file to test
#include "SharedFlags.h"

namespace fs = std::filesystem;

class UtilsTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("UtilsTest");
    }

    static void TearDownTestSuite() {
    }
};

TEST(UtilsTest, ReadFile_Success) {
    std::string test_content = "Hello, world!";
    fs::path test_path = "test_file.txt";
    
    // Create a test file
    std::ofstream out(test_path);
    ASSERT_TRUE(out.is_open());
    out << test_content;
    out.close();
    
    // Test readFile function
    std::string result = catena::readFile(test_path);
    EXPECT_EQ(result, test_content);
    
    // Clean up
    fs::remove(test_path);
}

TEST(UtilsTest, ReadFile_FileNotFound) {
    fs::path test_path = "non_existent_file.txt";
    
    // Expect an exception since the file doesn't exist
    EXPECT_EQ(catena::readFile(test_path), "");
}

// SUBS TESTS

TEST(UtilsTest, Subs_NormalCase) {
    std::string str = "hello world, world!";
    catena::subs(str, "world", "everyone");
    EXPECT_EQ(str, "hello everyone, everyone!");
}

TEST(UtilsTest, Subs_NoMatch) {
    std::string str = "hello world";
    catena::subs(str, "foo", "bar");
    EXPECT_EQ(str, "hello world"); // No changes should be made
}

TEST(UtilsTest, Subs_EmptyString) {
    std::string str = "";
    catena::subs(str, "foo", "bar");
    EXPECT_EQ(str, ""); // No changes should be made
}

TEST(UtilsTest, Subs_ReplaceWithEmpty) {
    std::string str = "aaa bbb aaa";
    catena::subs(str, "aaa", "");
    EXPECT_EQ(str, " bbb ");
}

TEST(UtilsTest, Subs_EmptySearchString) {
    std::string str = "hello world";
    catena::subs(str, "", "bar");
    EXPECT_EQ(str, "hello world"); // No changes should be made
}


// SPLIT TESTS

TEST(UtilsTest, Split_NormalCase) {
    std::vector<std::string> out;
    std::string str = "comma,seperated,values";
    std::vector<std::string> ans = {"comma", "seperated", "values"};
    catena::split(out, str, ",");
    EXPECT_EQ(out, ans);
}

TEST(UtilsTest, Split_NoMatch) {
    std::vector<std::string> out;
    std::string str = "comma,seperated,values";
    std::vector<std::string> ans = {"comma,seperated,values"};
    catena::split(out, str, " ");
    EXPECT_EQ(out, ans);
}

TEST(UtilsTest, Split_EmptyDelim) {
    std::vector<std::string> out;
    std::string str = "comma,seperated,values";
    std::vector<std::string> ans = {"comma,seperated,values"};
    catena::split(out, str, "");
    EXPECT_EQ(out, ans);
}

TEST(UtilsTest, Split_OverwriteVector) {
    std::vector<std::string> out = {"some", "initial", "values"};
    std::string str = "comma,seperated,values";
    std::vector<std::string> ans = {"comma", "seperated", "values"};
    catena::split(out, str, ",");
    EXPECT_EQ(out, ans);
}

TEST(UtilsTest, Fmt_NormalCase) {
    std::string result = catena::fmt("Hello, %s! You have %d new messages.", "Alice", 5);
    EXPECT_EQ(result, "Hello, Alice! You have 5 new messages.");
}

TEST(UtilsTest, Fmt_EmptyFormat) {
    std::string result = catena::fmt("");
    EXPECT_EQ(result, "");
}

TEST(UtilsTest, Fmt_NoArgs) {
    std::string result = catena::fmt("Hello, World!");
    EXPECT_EQ(result, "Hello, World!");
}

TEST(UtilsTest, Value_Param_Empty) {
    st2138::Value value;
    st2138::Empty* emptyValue = new st2138::Empty();
    value.set_allocated_empty_value(emptyValue);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "");
}

TEST(UtilsTest, Value_Param_Int32) {
    st2138::Value value;
    value.set_int32_value(42);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "42");
}

TEST(UtilsTest, Value_Param_Float32) {
    st2138::Value value;
    value.set_float32_value(3.14f);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "3.140000");
}

TEST(UtilsTest, Value_Param_String) {
    st2138::Value value;
    value.set_string_value("test string");
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "test string");
}

TEST(UtilsTest, Value_Param_Struct) {
    st2138::Value value;
    value.set_allocated_struct_value(new st2138::StructValue());
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[struct value]");
}

TEST(UtilsTest, Value_Param_Int32Array) {
    st2138::Value value;
    auto int_array = value.mutable_int32_array_values();
    int_array->add_ints(1);
    int_array->add_ints(2);
    int_array->add_ints(3);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[1, 2, 3]");
}

TEST(UtilsTest, Value_Param_Float32Array) {
    st2138::Value value;
    auto float_array = value.mutable_float32_array_values();
    float_array->add_floats(1.1f);
    float_array->add_floats(2.2f);
    float_array->add_floats(3.3f);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[1.1, 2.2, 3.3]");
}

TEST(UtilsTest, Value_Param_StringArray) {
    st2138::Value value;
    auto string_array = value.mutable_string_array_values();
    string_array->add_strings("one");
    string_array->add_strings("two");
    string_array->add_strings("three");
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[\"one\", \"two\", \"three\"]");
}

TEST(UtilsTest, Value_Param_DataPayload) {
    st2138::Value value;
    st2138::DataPayload* dataPayload = new st2138::DataPayload();
    dataPayload->set_payload("binarydata");
    value.set_allocated_data_payload(dataPayload);
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "binarydata");
}

TEST(UtilsTest, Value_Param_StructVariantValue) {
    st2138::Value value;
    value.set_allocated_struct_variant_value(new st2138::StructVariantValue());
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[struct variant value]");
}

TEST(UtilsTest, Value_Param_StructVariantArrayValues) {
    st2138::Value value;
    value.set_allocated_struct_variant_array_values(new st2138::StructVariantList());
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[struct variant array values]");
}

TEST(UtilsTest, Value_Param_DefaultCase) {
    st2138::Value value; // No kind set
    std::string result = catena::param_value_string(value);
    EXPECT_EQ(result, "[no value]");
}