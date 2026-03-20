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

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class LoggerTest : public ::testing::Test {
    protected:
        static void clear_directory(const std::filesystem::path& path) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                std::filesystem::remove_all(entry.path());
            }
        }

        // Set up and tear down Google Logging
        static void SetUpTestSuite() {
            config::log_dir = UNITTEST_LOG_DIR + std::string("/logger");
            clear_directory(config::log_dir);
            config::log_console = true;
            config::log_file = true;
            config::log_level = "INFO";
            config::log_size = 0.001;
            config::log_count = 1;
            config::log_verbosity = 2;
            config::silent = false;
            Logger::init("LoggerTest");
        }

        void SetUp() override {
            //wipe entire UNITTEST_LOG_DIR logging dir
        }

        void rotate_log_file(int bytes){
            int bytes_to_add = config::log_size * 1024 * 1024 - bytes - 100;
            const std::string padding(bytes_to_add, 'a');
            LOG(INFO) << padding;
            boost::log::core::get()->flush();
        }
};

// Test 1: Normal case
TEST_F(LoggerTest, LogDefaultDir) {
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";

    //verify that the log file was created and contains the log messages
    std::string logFilePattern = "_LoggerTest.log"; // Log file name starts with this
    bool foundLogFile = false;
    int fileCount = 0;

    int size = 0;
    for (const auto& entry : std::filesystem::directory_iterator(UNITTEST_LOG_DIR + std::string("/logger"))) {
        if (entry.is_regular_file()) {
            fileCount++;
            size = entry.file_size();
        }

        //get the file that ends with _LoggerTest
        if (entry.is_regular_file() && entry.path().filename().string().find(logFilePattern) == entry.path().filename().string().length() - logFilePattern.length()) {
            foundLogFile = true;
            std::ifstream logFile(entry.path());
            std::string logContent((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
            EXPECT_NE(logContent.find("This is an info log message."), std::string::npos);
            EXPECT_NE(logContent.find("This is a warning log message."), std::string::npos);
            EXPECT_NE(logContent.find("This is an error log message."), std::string::npos);
            EXPECT_TRUE(std::regex_match(entry.path().filename().string(), std::regex(R"(^\d{8}_\d{6}_LoggerTest[.]log$)")));
            break;
        }
    }

    //fail test if the directory has more than 1 file
    EXPECT_EQ(fileCount, 1);

    rotate_log_file(size);
}

// Test 2: Severity filter only passes severity >= minimum
TEST_F(LoggerTest, LogSeverityFilter) {
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

    //verify that the log file was created and contains the log messages
    std::string logFilePattern = "_LoggerTest.log"; // Log file name starts with this
    bool foundLogFile = false;
    int fileCount = 0;

    int size = 0;
    for (const auto& entry : std::filesystem::directory_iterator(UNITTEST_LOG_DIR + std::string("/logger"))) {
        if (entry.is_regular_file()) {
            fileCount++;
            size = entry.file_size();
        }

        //get the file that ends with _LoggerTest
        if (entry.is_regular_file() && entry.path().filename().string().find(logFilePattern) == entry.path().filename().string().length() - logFilePattern.length()) {
            foundLogFile = true;
            std::ifstream logFile(entry.path());
            std::regex severity_re(R"(^\d+:\s*(info|warning|error)\s)");
            std::smatch m;
            int info_count = 0, warning_count = 0, error_count = 0;

            for (std::string line; std::getline(logFile, line); ) {
                if (std::regex_search(line, m, severity_re)) {
                    std::string sev = m[1].str();
                    if (sev == "info")    ++info_count;
                    if (sev == "warning") ++warning_count;
                    if (sev == "error")   ++error_count;
                }
            }
            EXPECT_EQ(info_count, 1);
            EXPECT_EQ(warning_count, 2);
            EXPECT_EQ(error_count, 3);
            break;
        }
    }

    //fail test if the directory has more than 1 file
    EXPECT_EQ(fileCount, 1);
    config::log_level = "INFO";
    rotate_log_file(size);
}

// Test 3: Verbosity filter only passes verbosity <= maximum
TEST_F(LoggerTest, LogVerbosityFilter) {
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

    //verify that the log file was created and contains the log messages
    std::string logFilePattern = "_LoggerTest.log"; // Log file name starts with this
    bool foundLogFile = false;
    int fileCount = 0;

    int size = 0;
    for (const auto& entry : std::filesystem::directory_iterator(UNITTEST_LOG_DIR + std::string("/logger"))) {
        if (entry.is_regular_file()) {
            fileCount++;
            size = entry.file_size();
        }

        //get the file that ends with _LoggerTest
        if (entry.is_regular_file() && entry.path().filename().string().find(logFilePattern) == entry.path().filename().string().length() - logFilePattern.length()) {
            foundLogFile = true;
            std::ifstream logFile(entry.path());
            std::regex verbosity_re(R"(level ([012]))");
            std::smatch m;
            int lvl_0 = 0, lvl_1 = 0, lvl_2 = 0;

            for (std::string line; std::getline(logFile, line); ) {
                if (std::regex_search(line, m, verbosity_re)) {
                    std::string verb = m[1].str();
                    if (verb == "0")    ++lvl_0;
                    if (verb == "1") ++lvl_1;
                    if (verb == "2")   ++lvl_2;
                }
            }
            EXPECT_EQ(lvl_0, 3);
            EXPECT_EQ(lvl_1, 2);
            EXPECT_EQ(lvl_2, 1);
            break;
        }
    }

    //fail test if the directory has more than 1 file
    EXPECT_EQ(fileCount, 1);
    rotate_log_file(size);
}

// Test 4: Silence prevents all messages
TEST_F(LoggerTest, LogSilence) {
    LOG(INFO) << "ROTATE";
    config::silent = true;
    LOG(INFO) << "MESSAGE";

    //verify that the log file was created and does not contain the log messages
    std::string logFilePattern = "_LoggerTest.log"; // Log file name starts with this
    bool foundLogFile = false;
    int fileCount = 0;

    for (const auto& entry : std::filesystem::directory_iterator(UNITTEST_LOG_DIR + std::string("/logger"))) {
        if (entry.is_regular_file()) {
            fileCount++;
        }

        //get the file that ends with _LoggerTest
        if (entry.is_regular_file() && entry.path().filename().string().find(logFilePattern) == entry.path().filename().string().length() - logFilePattern.length()) {
            foundLogFile = true;
            std::ifstream logFile(entry.path());
            std::string logContent((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
            EXPECT_EQ(logContent.find("MESSAGE"), std::string::npos);
            EXPECT_TRUE(std::regex_match(entry.path().filename().string(), std::regex(R"(^\d{8}_\d{6}_LoggerTest[.]log$)")));
            break;
        }
    }

    //fail test if the directory has more than 1 file
    EXPECT_EQ(fileCount, 1);
    config::silent = false;
}
