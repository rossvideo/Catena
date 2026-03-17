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

#include "absl/log/log.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "absl/log/log_entry.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/globals.h"
#include "absl/flags/usage.h"
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

//BOOST libraries
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

// Helper to get basename of __FILE__
inline const char* log_basename(const char* path) {
    const char* p = std::strrchr(path, '/');
    if (!p) p = std::strrchr(path, '\\');
    return p ? p + 1 : path;
}

// Helper to get kernel id of thread
inline int kernel_thread_id() {
  return static_cast<int>(gettid());
}

static unsigned int current_id = 0;
inline unsigned int get_record_id() {
  return ++current_id;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(IsVlog, "IsVlog", bool);
#define CATENA_LOG(severity) \
  BOOST_LOG_SCOPED_THREAD_ATTR("IsVlog", boost::log::attributes::constant<bool>(false)); \
  BOOST_LOG_TRIVIAL(severity) << boost::log::add_value("File", log_basename(__FILE__)) \
                              << boost::log::add_value("Line", __LINE__) \
                              << boost::log::add_value("KernelThreadID", kernel_thread_id()) \
                              << boost::log::add_value("RecordID", get_record_id())
#define CATENA_VLOG(severity) \
  BOOST_LOG_SCOPED_THREAD_ATTR("IsVlog", boost::log::attributes::constant<bool>(true)); \
  BOOST_LOG_TRIVIAL(severity) << boost::log::add_value("File", log_basename(__FILE__)) \
                              << boost::log::add_value("Line", __LINE__) \
                              << boost::log::add_value("KernelThreadID", kernel_thread_id()) \
                              << boost::log::add_value("RecordID", get_record_id())

/**
 * @brief A log sink that writes log messages to a specified file.
 */
class FileLogSink : public absl::LogSink {
public:
  explicit FileLogSink(const std::string& filename) : log_file_(filename, std::ios_base::app) {
    if (!log_file_.is_open()) {
      std::cerr << "Error opening log file: " << filename << std::endl;
    }
  }

  void Send(const absl::LogEntry& entry) override {
    if (log_file_.is_open()) {
      log_file_ << entry.text_message_with_prefix_and_newline();
      log_file_.flush(); // Ensure immediate write to file
    }
  }

private:
  std::ofstream log_file_;
};

typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> file_sink_t;
typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> console_sink_t;

/**
 * @brief Logger class for logging messages to both console and file.
 */
class Logger {
public:
  /**
  * @brief Initialize the logger.
  * @param appName The name of the application.
  */
  static void init(const std::string& appName);

  ~Logger() {
    if (fileLogSink_) {
      absl::RemoveLogSink(fileLogSink_.get());
    }
  }

private:
  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  std::ostringstream stream_;
  std::unique_ptr<FileLogSink> fileLogSink_ = nullptr;
};

/**
 * @brief Logger class for logging messages to both console and file.
 */
class Logger2 {
public:
  /**
  * @brief Initialize the logger.
  * @param appName The name of the application.
  */
  static void init(const std::string& appName);

  ~Logger2() {
    auto core = boost::log::core::get();
    core->flush();
    core->remove_all_sinks();
  }

private:
  static Logger2& instance() {
    static Logger2 logger;
    return logger;
  }

  Logger2() = default;
  Logger2(const Logger2&) = delete;
  Logger2& operator=(const Logger2&) = delete;

  boost::shared_ptr<file_sink_t> file_sink_;
  boost::shared_ptr<console_sink_t> console_sink_;
};
