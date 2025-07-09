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

namespace fs = std::filesystem;

class UtilsTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("UtilsTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
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
    EXPECT_THROW(catena::readFile(test_path), std::filesystem::filesystem_error);
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
