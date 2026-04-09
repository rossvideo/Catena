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
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-03-20
 */

#include <mutex>

//BOOST libraries
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

// Helper functions used by the LOG() macro
namespace LogHelper{
  // Helper to get basename of __FILE__
  const char* log_basename(const char* path);

  // Helper to get kernel id of thread
  int kernel_thread_id();

  // Helper for log file line numbers
  inline std::atomic<unsigned int> current_record_id = 0;
  unsigned int get_record_id();
}

// This map is to keep the boost system consistent with the old abseil system which uses all-caps for severity
#define CATENA_SEV_(x)    CATENA_SEV_##x
#define CATENA_SEV_TRACE   trace
#define CATENA_SEV_DEBUG   debug
#define CATENA_SEV_INFO   info
#define CATENA_SEV_WARNING warning
#define CATENA_SEV_ERROR  error
#define CATENA_SEV_FATAL  fatal
    
#define LOG_IMPL(severity) \
  BOOST_LOG_TRIVIAL(severity) \
    << boost::log::add_value("File", LogHelper::log_basename(__FILE__)) \
    << boost::log::add_value("Line", __LINE__) \
    << boost::log::add_value("KernelThreadID", LogHelper::kernel_thread_id()) \
    << boost::log::add_value("RecordID", LogHelper::get_record_id())

#define LOG(severity) \
  LOG_IMPL(CATENA_SEV_(severity))

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

  /**
   * @brief Reset the logger
   * 
   * Must be called before re-initializing the Logger.
   * Intended for use in the unit tests, shouldn't be needed for production.
   * If used, ensure that no threads are currently logging or can log when called.
   */
  static void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
      instance().teardown();
      initialized_ = false;
    }
  }

  ~Logger() {
    teardown();
  }

private:
  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void teardown(){
      auto core = boost::log::core::get();
      core->flush();
      core->remove_all_sinks();
      instance().file_sink_.reset();
      instance().console_sink_.reset();
  }

  boost::shared_ptr<file_sink_t> file_sink_;
  boost::shared_ptr<console_sink_t> console_sink_;

  static bool initialized_;
  static std::mutex mutex_;
};
