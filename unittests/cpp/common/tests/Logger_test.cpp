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

        static std::filesystem::path FindPath() {
            constexpr std::string_view suffix = "_LoggerTest.log";
            for (const auto& entry : std::filesystem::directory_iterator(config::log_dir)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                const std::string fn = entry.path().filename().string();
                if (fn.size() >= suffix.size() &&
                    fn.compare(fn.size() - suffix.size(), suffix.size(), suffix) == 0) {
                    return entry.path();
                }
            }
            return {};
        }

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

        // Set up and tear down Google Logging
        static void SetUpTestSuite() {
            config::log_dir = UNITTEST_LOG_DIR + std::string("/logger");
            config::log_console = true;
            config::log_file = true;
            config::log_level = "INFO";
            config::log_size = 10;
            config::log_count = 1;
            config::log_verbosity = 2;
            config::silent = false;
            Logger::init("LoggerTest");
        }
};

// Test 1: Normal case
TEST_F(LoggerTest, LogDefaultDir) {
    // Log test messages
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogDefaultDir>>>";
    LOG(INFO) << marker;
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";

    boost::log::core::get()->flush();

    // Verify file created properly
    const std::filesystem::path logPath = FindPath();
    ASSERT_FALSE(logPath.empty());
    EXPECT_TRUE(std::regex_match(logPath.filename().string(),
                                 std::regex(R"(^\d{8}_\d{6}_LoggerTest[.]log$)")));

    const std::string fullLog = ReadFile(logPath);
    const std::string slice = TestSlice(fullLog, marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";
    // Check messages were written correctly
    EXPECT_NE(slice.find("This is an info log message."), std::string::npos);
    EXPECT_NE(slice.find("This is a warning log message."), std::string::npos);
    EXPECT_NE(slice.find("This is an error log message."), std::string::npos);

    EXPECT_EQ(CountFiles(), 1);
}

// Test 2: Severity filter only passes severity >= minimum
TEST_F(LoggerTest, LogSeverityFilter) {
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogSeverityFilter>>>";
    LOG(INFO) << marker;

    // Log test messages at different severity levels
    config::log_level = "INFO";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    config::log_level = "WARNING";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";
    config::log_level = "ERROR";
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";

    boost::log::core::get()->flush();

    const std::filesystem::path logPath = FindPath();
    ASSERT_FALSE(logPath.empty());
    const std::string slice = TestSlice(ReadFile(logPath), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";

    // Check messages were written in the right quantity
    std::regex severity_re(R"(^\d+:\s*(info|warning|error)\s)");
    std::smatch m;
    int info_count = 0, warning_count = 0, error_count = 0;
    std::istringstream stream(slice);
    for (std::string line; std::getline(stream, line);) {
        if (std::regex_search(line, m, severity_re)) {
            const std::string& sev = m[1].str();
            if (sev == "info") {
                ++info_count;
            }
            if (sev == "warning") {
                ++warning_count;
            }
            if (sev == "error") {
                ++error_count;
            }
        }
    }
    EXPECT_EQ(info_count, 1);
    EXPECT_EQ(warning_count, 2);
    EXPECT_EQ(error_count, 3);

    EXPECT_EQ(CountFiles(), 1);
    config::log_level = "INFO";
}

// Test 3: Verbosity filter only passes verbosity <= maximum
TEST_F(LoggerTest, LogVerbosityFilter) {
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogVerbosityFilter>>>";
    LOG(INFO) << marker;

    // Log test messages at different verbosity levels
    config::log_verbosity = 0;
    VLOG(0) << "level 0";
    VLOG(1) << "level 1";
    VLOG(2) << "level 2";
    config::log_verbosity = 1;
    VLOG(0) << "level 0";
    VLOG(1) << "level 1";
    VLOG(2) << "level 2";
    config::log_verbosity = 2;
    VLOG(0) << "level 0";
    VLOG(1) << "level 1";
    VLOG(2) << "level 2";

    boost::log::core::get()->flush();

    const std::filesystem::path logPath = FindPath();
    ASSERT_FALSE(logPath.empty());
    const std::string slice = TestSlice(ReadFile(logPath), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";

    // Check messages were written in the right quantity
    std::regex verbosity_re(R"(level ([012]))");
    std::smatch m;
    int lvl_0 = 0, lvl_1 = 0, lvl_2 = 0;
    std::istringstream stream(slice);
    for (std::string line; std::getline(stream, line);) {
        if (std::regex_search(line, m, verbosity_re)) {
            const std::string& verb = m[1].str();
            if (verb == "0") {
                ++lvl_0;
            }
            if (verb == "1") {
                ++lvl_1;
            }
            if (verb == "2") {
                ++lvl_2;
            }
        }
    }
    EXPECT_EQ(lvl_0, 3);
    EXPECT_EQ(lvl_1, 2);
    EXPECT_EQ(lvl_2, 1);

    EXPECT_EQ(CountFiles(), 1);
    config::log_verbosity = 2;
}

// Test 4: Silence prevents all messages
TEST_F(LoggerTest, LogSilence) {
    const std::string marker = "<<<CATENA_LOGGER_TEST: LogSilence>>>";
    LOG(INFO) << marker;

    // Log test message which shouldn't be written
    config::silent = true;
    LOG(INFO) << "MESSAGE";

    boost::log::core::get()->flush();

    const std::filesystem::path logPath = FindPath();
    ASSERT_FALSE(logPath.empty());
    const std::string slice = TestSlice(ReadFile(logPath), marker);
    ASSERT_FALSE(slice.empty()) << "Test marker not found in log";
    // Check message not in file
    EXPECT_EQ(slice.find("MESSAGE"), std::string::npos);

    EXPECT_EQ(CountFiles(), 1);
    config::silent = false;
}
