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
 * @brief This file is for testing the Logger.cpp file.
 * @author christian.twarog@rossvideo.com
 * @date 2025-10-01
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "Authorizer.h"
#include "Enums.h"
#include "CommonTestHelpers.h"
#include <Logger.h>
#include <SharedFlags.h>
#include <regex>

// gtest
#include <gtest/gtest.h>

using namespace catena::common;

class LoggerTest : public ::testing::Test {
    void SetUp() override {
        //wipe entire UNITTEST_LOG_DIR logging dir
        std::filesystem::remove_all(UNITTEST_LOG_DIR + std::string("/logger"));

        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR + std::string("/logger"));
        Logger::init("LoggerTest");
    }
};

TEST_F(LoggerTest, LogDefaultDir) {
    LOG(INFO) << "This is an info log message.";
    LOG(WARNING) << "This is a warning log message.";
    LOG(ERROR) << "This is an error log message.";

    //verify that the log file was created and contains the log messages
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
            EXPECT_NE(logContent.find("This is an info log message."), std::string::npos);
            EXPECT_NE(logContent.find("This is a warning log message."), std::string::npos);
            EXPECT_NE(logContent.find("This is an error log message."), std::string::npos);
            EXPECT_TRUE(std::regex_match(entry.path().filename().string(), std::regex(R"(^\d{8}_\d{6}_LoggerTest[.]log$)")));
            break;
        }
    }

    //fail test if the directory has more than 1 file
    EXPECT_EQ(fileCount, 1);
}
