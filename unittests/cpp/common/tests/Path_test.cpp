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

/*
 * TEST 1 - Testing Path constructor with valid paths. 
 */
TEST(PathTest, Path_CreateValid) {
    std::vector<std::string> paths = {
        "/test", "/test/path", "/0", "/test/0", "/test/0/path", "/-", "/test/-"
    };
    for (std::string& path : paths) {
        Path p(path);
        EXPECT_EQ(p.fqoid(), path);
    }
}
/*
 * TEST 2 - Testing Path constructor with invalid paths. 
 */
TEST(PathTest, Path_CreateInvalid) {
    std::vector<std::string> paths = {
        "/1test", "/test/1path", "/test-path", "/test//path", "test/path"
    };
    for (std::string& path : paths) {
        EXPECT_THROW(Path p(path), catena::exception_with_status) << "Path \"" << path << "\" should throw an exception.";
    }
}
/*
 * TEST 3 - Testing Path literal constructor. 
 */
TEST(PathTest, Path_CreateLiteral) {
     std::vector<std::string> paths = {
        "/test", "/test/path", "/0", "/test/0", "/test/0/path", "/-",
        "/test/-", "/1test", "/test/1path", "/test-path", "/test//path"
    };
    for (std::string& path : paths) {
        Path p{path};
        EXPECT_EQ(p.toString(false), path);
    }
}
/*
 * TEST 4 - Testing Path pop(), size(), walked(), unpop(), rewind(), and empty(). 
 */
TEST(PathTest, Path_Pop) {
    // Initializing path.
    Path p("/test/path/1/-");
    Path::Index size = p.size();
    EXPECT_EQ(p.walked(), 0)          << "Walked should be 0 before popping";
    // Tesing unpop() before pop().
    p.unpop();
    EXPECT_EQ(p.walked(), 0)          << "Unpop should not change walked segments before popping";
    EXPECT_EQ(p.size(), size)         << "Unpop should not change size before popping";
    // Testing pop() on all segments.
    EXPECT_FALSE(p.empty())           << "Path should not be empty before popping";
    for (int32_t i = 0; i < size; ++i) {
        EXPECT_EQ(p.walked(), i)      << "Walked segments should be "<<i<<" on pop "<<i;
        EXPECT_EQ(p.size(), size - i) << "Path size should be "<<(size - i)<<" on pop "<<i;
        p.pop();
    }
    EXPECT_EQ(p.walked(), size)       << "Walked segments should be "<<size<<" on final pop";
    EXPECT_TRUE(p.empty())            << "Path should be empty after popping all segments";
    // Testing pop() on empty path,
    p.pop();
    EXPECT_EQ(p.walked(), size)       << "Walked segments should still be "<<size<<" after popping empty path";
    EXPECT_TRUE(p.empty())            << "Path should still be empty after popping empty path";
    // Testing unpop() after pop().
    p.unpop();
    EXPECT_EQ(p.walked(), 3)          << "Walked segments should be "<<size - 1<<" after unpop";
    EXPECT_EQ(p.size(), 1)            << "Path size should be 1 after unpop";
    EXPECT_FALSE(p.empty())           << "Path should not be empty after unpop";
    // Testing rewind().
    p.rewind();
    EXPECT_EQ(p.walked(), 0)          << "Walked segments should be 0 after rewind";
    EXPECT_EQ(p.size(), size)         << "Path size should be "<<size<<" after rewind";
    EXPECT_FALSE(p.empty())           << "Path should not be empty after rewind";
}
/*
 * TEST 5 - Testing Path popBack(), size(), walked(), and empty(). 
 */
TEST(PathTest, Path_PopBack) {
    // Initializing path.
    Path p("/test/path/1/-");
    Path::Index size = p.size();
    p.pop();
    EXPECT_EQ(p.walked(), 1)              << "Walked should be 1 before popping back";
    // Testing popBack() on all segments.
    for (int32_t i = 0; i < size; ++i) {
        EXPECT_EQ(p.walked(), 1)          << "popBack should not increment frontIdx_";
        EXPECT_EQ(p.size(), size - i - 1) << "Path size should be "<<(size - i - 1)<<" on pop "<<i;
        p.popBack();
    }
    EXPECT_EQ(p.walked(), 0)              << "Walked segments should be 0 on final popBack";
    EXPECT_TRUE(p.empty())                << "Path should be empty after popping all segments";
    // Testing pop() on empty path,
    p.popBack();
    EXPECT_EQ(p.walked(), 0)              << "Walked segments should still be "<<0<<" after popping empty path";
    EXPECT_TRUE(p.empty())                << "Path should still be empty after popping empty path";
}
/*
 * TEST 6 - Testing Path FrontIsString(), FrontIsIndex(), FrontAsString(),
 *          FrontAsIndex() with string segments.
 */
TEST(PathTest, Path_FrontIsAsString) {
    std::vector<std::string> segments = { "test", "path" };
    Path p("/test/path");
    for (auto& segment : segments) {
        EXPECT_TRUE(p.front_is_string())            << "Front of path \""<<p.fqoid()<<"\" is a string";
        EXPECT_FALSE(p.front_is_index())            << "Front of path \""<<p.fqoid()<<"\" is not an index";
        EXPECT_EQ(p.front_as_string(), segment)     << "Front of path \""<<p.fqoid()<<"\" as string should be \""<<segment<<"\"";
        EXPECT_EQ(p.front_as_index(), Path::kError) << "Front of path \""<<p.fqoid()<<"\" as index should return kError";
        p.pop();
    }
    EXPECT_TRUE(p.empty())                          << "Path should be empty after popping all segments";
    EXPECT_FALSE(p.front_is_string())               << "Front of path \"/test/path\" should not be a string after popping all segments";
}
/*
 * TEST 7 - Testing Path FrontIsString(), FrontIsIndex(), FrontAsString(),
 *          FrontAsIndex() with index segments. 
 */
TEST(PathTest, Path_FrontIsAsIndex) {
    using Segments = std::vector<Path::Index>;
    std::vector<std::pair<std::string, Segments>> tests = {
        {"/1/2/3", {1, 2, 3}}, {"/-", {Path::kEnd}}
    };
    for (auto& [fqoid, segments] : tests) {
        Path p(fqoid);
        for (auto& segment : segments) {
            EXPECT_TRUE(p.front_is_index())        << "Front of path \""<<p.fqoid()<<"\" is an index";
            EXPECT_FALSE(p.front_is_string())      << "Front of path \""<<p.fqoid()<<"\" is not a string";
            EXPECT_EQ(p.front_as_index(), segment) << "Front of path \""<<p.fqoid()<<"\" as index should be \""<<segment<<"\"";
            EXPECT_EQ(p.front_as_string(), "")     << "Front of path \""<<p.fqoid()<<"\" as string should return \"\"";
            p.pop();
        }
        EXPECT_TRUE(p.empty())                     << "Path should be empty after popping all segments";
        EXPECT_FALSE(p.front_is_index())           << "Front of path \""<<fqoid<<"\" should not be an index after popping all segments";
    }
}
/*
 * TEST 8 - Testing Path BackIsString(), BackIsIndex(), BackAsString(),
 *          BackAsIndex() with string segments.
 */
TEST(PathTest, Path_BackIsAsString) {
    std::vector<std::string> segments = { "path", "test" };
    Path p("/test/path");
    for (auto& segment : segments) {
        EXPECT_TRUE(p.back_is_string())            << "Back of path \""<<p.fqoid()<<"\" is a string";
        EXPECT_FALSE(p.back_is_index())            << "Back of path \""<<p.fqoid()<<"\" is not an index";
        EXPECT_EQ(p.back_as_string(), segment)     << "Back of path \""<<p.fqoid()<<"\" as string should be \""<<segment<<"\"";
        EXPECT_EQ(p.back_as_index(), Path::kError) << "Back of path \""<<p.fqoid()<<"\" as index should return kError";
        p.popBack();
    }
    EXPECT_TRUE(p.empty())                         << "Path should be empty after popping all segments";
    EXPECT_FALSE(p.back_is_string())               << "Back of path \"/test/path\" should not be a string after popping all segments";
}
/*
 * TEST 9 - Testing Path BackIsString(), BackIsIndex(), BackAsString(),
 *          BackAsIndex() with index segments. 
 */
TEST(PathTest, Path_BackIsAsIndex) {
    using Segments = std::vector<Path::Index>;
    std::vector<std::pair<std::string, Segments>> tests = {
        {"/1/2/3", {3, 2, 1}}, {"/-", {Path::kEnd}}
    };
    for (auto& [fqoid, segments] : tests) {
        Path p(fqoid);
        for (auto& segment : segments) {
            EXPECT_TRUE(p.back_is_index())        << "Back of path \""<<p.fqoid()<<"\" is an index";
            EXPECT_FALSE(p.back_is_string())      << "Back of path \""<<p.fqoid()<<"\" is not a string";
            EXPECT_EQ(p.back_as_index(), segment) << "Back of path \""<<p.fqoid()<<"\" as index should be \""<<segment<<"\"";
            EXPECT_EQ(p.back_as_string(), "")     << "Back of path \""<<p.fqoid()<<"\" as string should return \"\"";
            p.popBack();
        }
        EXPECT_TRUE(p.empty())                    << "Path should be empty after popping all segments";
        EXPECT_FALSE(p.back_is_index())           << "Back of path \""<<fqoid<<"\" should not be an index after popping all segments";
    }
}
/*
 * TEST 10 - Testing Path toString() and fqoid()
 */
TEST(PathTest, Path_ToString) {
    Path p("/test/path/1/-");
    // Testing before pop().
    EXPECT_EQ(p.toString(true), "/test/path/1/-") << "toString(true) should have a leading slash";
    EXPECT_EQ(p.toString(false), "test/path/1/-") << "toString(false) should not have a leading slash";
    EXPECT_EQ(p.fqoid(), "/test/path/1/-")        << "fqoid should have a leading slash";
    // Testing after pop().
    p.pop();
    EXPECT_EQ(p.toString(true), "/path/1/-")      << "toString(true) should start from frontInx_ and have a leading slash";
    EXPECT_EQ(p.toString(false), "path/1/-")      << "toString(false) should start from frontInx_ and not have a leading slash";
    EXPECT_EQ(p.fqoid(), "/test/path/1/-")        << "fqoid should not change after pop()";
}
/*
 * TEST 11 - Testing Path push_back()
 */
TEST(PathTest, Path_PushBack) {
    Path p("/test/path");
    // Pushing back a string segment.
    p.push_back("new_segment");
    EXPECT_TRUE(p.back_is_string())                    << "Back of path should be a string after pushing back string segment";
    EXPECT_EQ(p.back_as_string(), "new_segment")       << "Back of path should be a \"new_segment\" after pushing back string segment";
    EXPECT_EQ(p.fqoid(), "/test/path/new_segment")     << "fqoid should be updated after pushing back string segment";
    // Pushing back an index segment.
    p.push_back(1);
    EXPECT_TRUE(p.back_is_index())                     << "Back of path should be an index after pushing back index segment";
    EXPECT_EQ(p.back_as_index(), 1)                    << "Back of path should be a 1 after pushing back index segment";
    EXPECT_EQ(p.fqoid(), "/test/path/new_segment/1")   << "fqoid should be updated after pushing back index segment";
    // Pushing back a kEnd segment.
    p.push_back("-");
    EXPECT_TRUE(p.back_is_index())                     << "Back of path should be an index after pushing back kEnd segment";
    EXPECT_EQ(p.back_as_index(), Path::kEnd)           << "Back of path should be kEnd after pushing back kEnd segment";
    EXPECT_EQ(p.fqoid(), "/test/path/new_segment/1/-") << "fqoid should be updated after pushing back kEnd segment";
}
/*
 * TEST 12 - Testing Path user-defined literal operator.
 */
TEST(PathTest, Path_Operator) {
    Path p1 = "/test/path/1/-"_path;
    Path p2("/test/path/1/-");
    EXPECT_EQ(p1.fqoid(), p2.fqoid());
}
