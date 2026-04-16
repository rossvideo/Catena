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
 * @brief This file is for testing the Logger.cpp file.
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-03-20
 * @copyright Copyright © 2026 Ross Video Ltd
 */

#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "Authorizer.h"
#include "Enums.h"
#include "CommonTestHelpers.h"
#include <Logger.h>
#include <Config.h>
#include <regex>
#include <sstream>
#include <fstream>
#include <string_view>

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class LoggerTest : public ::testing::Test {
    protected:

        static std::string ReadFile(const std::filesystem::path& path) {
            std::ifstream f(path);
            return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        }

        // Slice of log file belonging to the test case
        static std::string TestSlice(const std::string& fullLog, const std::string& marker) {
            const size_t pos = fullLog.find(marker);
            if (pos == std::string::npos) {
                return {};
            }
            return fullLog.substr(pos + marker.size());
        }

        static int CountFiles() {
            int n = 0;
            for (const auto& entry :
                 std::filesystem::directory_iterator(config::log_dir)) {
                if (entry.is_regular_file()) {
                    ++n;
                }
            }
            return n;
        }

        static std::optional<std::filesystem::path> NewestFile(const std::filesystem::path& dir) {
            std::optional<std::filesystem::path> newest;
            std::optional<std::filesystem::file_time_type> new_time;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                auto t = std::filesystem::last_write_time(entry);
                if (!new_time || t > *new_time) {
                    new_time = t;
                    newest = entry.path();
                }
            }
            return newest;
        }

        // Set up and tear down Google Logging
        static void SetUpTestSuite() {
            config::log_dir = UNITTEST_LOG_DIR + std::string("/logger");
            std::filesystem::create_directory(config::log_dir);
            config::log_console = true;
            config::log_file = true;
            config::log_level = "TRACE";
            config::log_size = 10;
            config::log_count = 1;
            config::silent = false;
            Logger::init("LoggerTest");
        }

};

// Test 1: Normal case
TEST_F(LoggerTest, LogDefaultDir) {
    std::string activeFile = config::log_dir + "/LoggerTest.log";
    // Log test messages
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogDefaultDir>>>";
    LOG(INFO) << marker;
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";

    // Verify file created properly
    EXPECT_TRUE(std::filesystem::exists(activeFile));

    const std::string slice = TestSlice(ReadFile(activeFile), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";
    // Check messages were written correctly
    EXPECT_NE(slice.find("This is a trace log message."), std::string::npos);
    EXPECT_NE(slice.find("This is a debug log message."), std::string::npos);
    EXPECT_NE(slice.find("This is an info log message."), std::string::npos);
    EXPECT_NE(slice.find("This is a warning log message."), std::string::npos);
    EXPECT_NE(slice.find("This is an error log message."), std::string::npos);
    EXPECT_NE(slice.find("This is a fatal log message."), std::string::npos);

    EXPECT_EQ(CountFiles(), 1);
}

// Test 2: Severity filter only passes severity >= minimum
TEST_F(LoggerTest, LogSeverityFilter) {
    std::string activeFile = config::log_dir + "/LoggerTest.log";
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogSeverityFilter>>>";
    LOG(INFO) << marker;

    // Log test messages at different severity levels
    config::log_level = "TRACE";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";
    config::log_level = "DEBUG";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";
    config::log_level = "INFO";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";
    config::log_level = "WARNING";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";
    config::log_level = "ERROR";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";
    config::log_level = "FATAL";
    LOG(TRACE) << "This is a trace log message.";
    LOG(DEBUG) << "This is a debug log message.";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    LOG(FATAL) << "This is a fatal log message.";

    const std::string slice = TestSlice(ReadFile(activeFile), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";

    // Check messages were written in the right quantity
    std::regex severity_re(R"(\s*(trace|debug|info|warning|error|fatal)\s)");
    std::smatch m;
    int trace_count = 0, debug_count = 0, info_count = 0, warning_count = 0, error_count = 0, fatal_count = 0;
    std::istringstream stream(slice);
    for (std::string line; std::getline(stream, line);) {
        if (std::regex_search(line, m, severity_re)) {
            const std::string& sev = m[1].str();
            if (sev == "trace") {
                ++trace_count;
            } else if (sev == "debug") {
                ++debug_count;
            } else if (sev == "info") {
                ++info_count;
            } else if (sev == "warning") {
                ++warning_count;
            } else if (sev == "error") {
                ++error_count;
            } else if (sev == "fatal") {
                ++fatal_count;
            }
        }
    }
    EXPECT_EQ(trace_count, 1);
    EXPECT_EQ(debug_count, 2);
    EXPECT_EQ(info_count, 3);
    EXPECT_EQ(warning_count, 4);
    EXPECT_EQ(error_count, 5);
    EXPECT_EQ(fatal_count, 6);

    EXPECT_EQ(CountFiles(), 1);
    config::log_level = "TRACE";
}

// Test 3: Silence prevents all messages
TEST_F(LoggerTest, LogSilence) {
    std::string activeFile = config::log_dir + "/LoggerTest.log";
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogSilence>>>";
    LOG(INFO) << marker;

    // Log test message which shouldn't be written
    config::silent = true;
    LOG(INFO) << "MESSAGE";

    const std::string slice = TestSlice(ReadFile(activeFile), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";
    // Check message not in file
    EXPECT_EQ(slice.find("MESSAGE"), std::string::npos);

    EXPECT_EQ(CountFiles(), 1);
    config::silent = false;
}

// Test 4: File Rotation
TEST_F(LoggerTest, Rotate) {
    // Some config variables are only evaluated upon initialization
    // so we must reset and reinitialize if we want to change them
    Logger::reset();
    config::log_dir = UNITTEST_LOG_DIR + std::string("/logger/rotate");
    std::string activeFile = config::log_dir + "/LoggerTest.log";
    std::filesystem::create_directory(config::log_dir);
    config::log_size = 0.00015; // ~0.15KB
    const uintmax_t max_bytes = config::log_size * 1024 * 1024;
    config::log_count = 3;
    Logger::init("LoggerTest");
    if (CountFiles() >= 1) {
        std::filesystem::remove_all(config::log_dir); // Clear any old files
        std::filesystem::create_directory(config::log_dir);
    }

    // First file
    LOG(INFO) << std::string(60, '_'); // Padding to fill up the file
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    EXPECT_EQ(CountFiles(), 1);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), max_bytes, 50); // Should be close to max (full file)
    
    LOG(INFO) << "ROTATE 1"; // This will cause a rotation and be written to a new file
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(CountFiles(), 2);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), 0, 65); // Should be close to zero (new file)
    
    // Second file
    LOG(INFO) << std::string(1, '/');
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(CountFiles(), 2);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), max_bytes, 50);
    
    LOG(INFO) << "ROTATE 2";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(CountFiles(), 3);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), 0, 65);

    // Third file
    LOG(INFO) << std::string(1, '|');
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(CountFiles(), 3);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), max_bytes, 50);
    
    LOG(INFO) << "ROTATE 3";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(CountFiles(), 3); // At max, oldest file is deleted upon rotation
    EXPECT_NEAR(std::filesystem::file_size(activeFile), 0, 65);
    
    // Check contents of final file
    std::string body = ReadFile(activeFile);
    EXPECT_NE(body.find("ROTATE 3"), std::string::npos);
}

// Test 5: Single File Rotation
TEST_F(LoggerTest, RotateSingleFile) {
    // Some config variables are only evaluated upon initialization
    // so we must reset and reinitialize if we want to change them
    Logger::reset();
    config::log_dir = UNITTEST_LOG_DIR + std::string("/logger/single");
    std::string activeFile = config::log_dir + "/LoggerTest.log";
    std::filesystem::create_directory(config::log_dir);
    config::log_size = 0.0001; // ~0.1KB
    const uintmax_t max_bytes = config::log_size * 1024 * 1024;
    config::log_count = 1;
    Logger::init("LoggerTest");

    LOG(INFO) << std::string(50, '-'); // Padding to fill up the file
    EXPECT_EQ(CountFiles(), 1);
    EXPECT_NEAR(std::filesystem::file_size(activeFile), max_bytes, 50);
    LOG(INFO) << "ROTATE 1"; // This will cause a rotation and be written to a new file
    EXPECT_EQ(CountFiles(), 1); // Number of files stays the same
    EXPECT_NEAR(std::filesystem::file_size(activeFile), 0, 65);

    // Check contents of rotated file
    std::string body = ReadFile(activeFile);
    EXPECT_NE(body.find("ROTATE 1"), std::string::npos);
}
