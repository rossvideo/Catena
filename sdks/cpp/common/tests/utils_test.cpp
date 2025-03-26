//  Copyright 2025 Ross Video Ltd
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//  
//  2. Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//  
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
// 
// 
//  this file is for testing the utils.cpp file
//
//  Author: nathan.rochon@rossvideo.com
//  Date: 25/03/26
// 

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

namespace fs = std::filesystem;

// int main(int argc, char** argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

#include "../src/utils.cpp" // Include the file to test

TEST(Utils, ReadFile_Success) {
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

TEST(Utils, ReadFile_FileNotFound) {
    fs::path test_path = "non_existent_file.txt";
    
    // Expect an exception since the file doesn't exist
    EXPECT_THROW(catena::readFile(test_path), std::filesystem::filesystem_error);
}


TEST(Utils, Subs_NormalCase) {
    std::string str = "hello world, world!";
    catena::subs(str, "world", "everyone");
    EXPECT_EQ(str, "hello everyone, everyone!");
}

TEST(Utils, Subs_NoMatch) {
    std::string str = "hello world";
    catena::subs(str, "foo", "bar");
    EXPECT_EQ(str, "hello world"); // No changes should be made
}

TEST(Utils, Subs_EmptyString) {
    std::string str = "";
    catena::subs(str, "foo", "bar");
    EXPECT_EQ(str, ""); // No changes should be made
}

TEST(Utils, Subs_ReplaceWithEmpty) {
    std::string str = "aaa bbb aaa";
    catena::subs(str, "aaa", "");
    EXPECT_EQ(str, " bbb ");
}

// TEST(Utils, Subs_EmptySearchString) {
//     std::string str = "hello world";
//     FAIL(); // This is an ifinite loop
//     catena::subs(str, "", "bar");
//     EXPECT_EQ(str, "hello world"); // No changes should be made
// }
