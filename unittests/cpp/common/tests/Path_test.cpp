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
 * @brief This file is for testing the Path.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// common
#include "Path.h"
#include "Status.h"

using namespace catena::common;

TEST(PathTest, Path_Create) {
    { // Valid
    std::vector<std::string> paths = {
        "/test", "/test/path", "/0", "/test/0", "/test/0/path", "/-", "/test/-"
    };
    for (std::string& path : paths) {
        Path p(path);
        EXPECT_EQ(p.fqoid(), path);
    }
    }
    { // Invalid
    std::vector<std::string> paths = {
        "/1test", "/test/1path", "/test-path", "/test//path"
    };
    for (std::string& path : paths) {
        EXPECT_THROW(Path p(path), catena::exception_with_status) << "Path \"" << path << "\" should throw an exception.";
    }
    }
    { // Literal
     std::vector<std::string> paths = {
        "/test", "/test/path", "/0", "/test/0", "/test/0/path", "/-",
        "/test/-", "/1test", "/test/1path", "/test-path", "/test//path"
    };
    for (std::string& path : paths) {
        Path p{path};
        EXPECT_EQ(p.toString(false), path);
    }
    }
}

TEST(PathTest, Path_FrontIsAsString) {
    Path p("/test/path");
    EXPECT_TRUE(p.front_is_string())            << "Front of path \"/test/path\" is a string";
    EXPECT_FALSE(p.front_is_index())            << "Front of path \"/test/path\" is not an index";
    EXPECT_EQ(p.front_as_string(), "test")      << "Front of path \"/test/path\" as string should be \"test\"";
    EXPECT_EQ(p.front_as_index(), Path::kError) << "Front of path \"/test/path\" as index should return kError";
}

TEST(PathTest, Path_FrontIsAsIndex) {
    std::vector<std::pair<std::string, Path::Index>> Paths = {
        {"/1/path", 1},    // Index
        {"/-", Path::kEnd} // kEnd
    };
    for (auto& [path, value] : Paths) {
        Path p(path);
        EXPECT_TRUE(p.front_is_index())        << "Front of path \""<<path<<"\" is an index";
        EXPECT_FALSE(p.front_is_string())      << "Front of path \""<<path<<"\" is not a string";
        EXPECT_EQ(p.front_as_index(), value)   << "Front of path \""<<path<<"\" as index should be "<<value;
        EXPECT_EQ(p.front_as_string(), "")     << "Front of path \""<<path<<"\" as string should return \"\"";
    }
}


// TEST(PathTest, Path_CreateString) {
//     { // Single segment
//     Path p("/test");
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Front segment of \"/test\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "test");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
//     { // Multiple segments
//     Path p("/test/path");
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Front segment of \"/test/path\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "test");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Second segment of \"/test/path\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "path");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
// }

// TEST(PathTest, Path_CreateIndex) {
//     { // Single segment
//     Path p("/0");
//     EXPECT_TRUE(p.front_is_index() && !p.front_is_string()) << "Front segment of \"/0\" should be index.";
//     EXPECT_EQ(p.front_as_index(), 0);
//     EXPECT_EQ(p.front_as_string(), "");
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
//     { // Multiple segments
//     Path p("/test/0");
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Front segment of \"/test/0\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "test");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.front_is_index() && !p.front_is_string()) << "Second segment of \"/test/0\" should be index.";
//     EXPECT_EQ(p.front_as_index(), 0);
//     EXPECT_EQ(p.front_as_string(), "");
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
//     { // Nested index
//     Path p("/test/0/path");
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Front segment of \"/test/0/path\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "test");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.front_is_index() && !p.front_is_string()) << "Second segment of \"/test/0/path\" should be index.";
//     EXPECT_EQ(p.front_as_index(), 0);
//     EXPECT_EQ(p.front_as_string(), "");
//     p.pop();
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Third segment of \"/test/0/path\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "path");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
// }

// TEST(PathTest, Path_CreateEnd) {
//     { // Single segment
//     Path p("/-");
//     EXPECT_TRUE(p.front_is_index() && !p.front_is_string()) << "Front segment of \"/0\" should be index.";
//     EXPECT_EQ(p.front_as_index(), Path::kEnd);
//     EXPECT_EQ(p.front_as_string(), "");
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
//     { // Multiple segments
//     Path p("/test/-");
//     EXPECT_TRUE(p.front_is_string() && !p.front_is_index()) << "Front segment of \"/test/-\" should be a string.";
//     EXPECT_EQ(p.front_as_string(), "test");
//     EXPECT_EQ(p.front_as_index(), Path::kError);
//     p.pop();
//     EXPECT_TRUE(p.front_is_index() && !p.front_is_string()) << "Second segment of \"/test/-\" should be index.";
//     EXPECT_EQ(p.front_as_index(), Path::kEnd);
//     EXPECT_EQ(p.front_as_string(), "");
//     p.pop();
//     EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
//     }
// }

TEST(PathTest, Path_CreateLiteral) {
    std::vector<std::string> paths = {"/test", "/test/path", "/test/path/1", "/test/path/-"};
    for (const std::string& pathStr : paths) {
        Path p{pathStr};
        EXPECT_TRUE(p.front_is_string() && !p.front_is_index());
        EXPECT_EQ(p.front_as_string(), pathStr);
        EXPECT_EQ(p.front_as_index(), Path::kError);
        p.pop();
        EXPECT_TRUE(p.empty()) << "Path should be empty after popping all segments.";
    }
}
