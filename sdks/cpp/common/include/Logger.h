#pragma once

/*
 * Copyright 2024 Ross Video Ltd
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
 * @brief Logger class.
 * @file Logger.h
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 */

#include <glog/logging.h>

#include <iostream>
#include <sstream>
#include <filesystem>

#ifdef NDEBUG
  // Release build: only log to file
  #define DEBUG_LOG LOG(INFO)
#else
  #define DEBUG_LOG Logger()
#endif

/**
 * @brief Logger class for logging messages to both console and file.
 */
class Logger {
public:
  Logger() = default;

  template <typename T>
  Logger& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

  static void StartLogging(const std::string& name) {
    FLAGS_logtostderr = false;          // Keep logging to files
    FLAGS_log_dir = GLOG_LOGGING_DIR;
    
    // Store the name in a static variable to ensure it stays alive
    // for the lifetime of the program, since glog stores the pointer
    static std::string program_name;
    program_name = name;
    google::InitGoogleLogging(program_name.c_str());
    std::cout << "[       ] Program output gets sent to " << GLOG_LOGGING_DIR << std::endl;
  }

  static void StartLogging(int argc, char** argv) {
    std::string name = "no_name";
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--silent") {
        FLAGS_minloglevel = 2;
      }
    }

    if (argc >= 1) {
      // Extract just the basename from argv[0] to avoid path separators in log filename
      std::filesystem::path execPath(argv[0]);
      name = execPath.filename().string();
    }

    StartLogging(name);
  }

  ~Logger() {
    std::string output = stream_.str();

    // Output to stdout
    std::cout << output << std::endl;

    // Output to glog file
    LOG(INFO) << output;
  }

private:
  std::ostringstream stream_;
};