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

#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

//BOOST libraries
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
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

// Helper for log file line numbers
inline std::atomic<unsigned int> current_record_id = 0;
inline unsigned int get_record_id() {
  return ++current_record_id;
}

/**
 * Helper struct for the verbosity levels
 * Can't be added like the other attributes due to filtering occuring
 * before record attributes are added, and this is needed for the filter itself
 */
struct ScopeVerbosity {
  boost::log::attribute_set::iterator attr;
  ScopeVerbosity(int v) {
    attr = boost::log::core::get()->add_thread_attribute("Verbosity", boost::log::attributes::constant<int>(v)).first;
  }
  ~ScopeVerbosity() {
    boost::log::core::get()->remove_thread_attribute(attr);
  }

  explicit operator bool() const { return true; }
};

// This map is to keep the boost system consistent with the old abseil system which uses all-caps for severity
#define CATENA_SEV_(x)    CATENA_SEV_##x
#define CATENA_SEV_INFO   info
#define CATENA_SEV_WARNING warning
#define CATENA_SEV_ERROR  error
    
#define LOG_IMPL(severity) \
  if (ScopeVerbosity v = ScopeVerbosity(0))\
  BOOST_LOG_TRIVIAL(severity) \
    << boost::log::add_value("File", log_basename(__FILE__)) \
    << boost::log::add_value("Line", __LINE__) \
    << boost::log::add_value("KernelThreadID", kernel_thread_id()) \
    << boost::log::add_value("RecordID", get_record_id())

#define LOG(severity) \
  LOG_IMPL(CATENA_SEV_(severity))

#define VLOG(verbosity) \
  if (ScopeVerbosity v = ScopeVerbosity(verbosity))\
  BOOST_LOG_TRIVIAL(info) \
    << boost::log::add_value("File", log_basename(__FILE__)) \
    << boost::log::add_value("Line", __LINE__) \
    << boost::log::add_value("KernelThreadID", kernel_thread_id()) \
    << boost::log::add_value("RecordID", get_record_id())

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
    auto core = boost::log::core::get();
    core->flush();
    core->remove_all_sinks();
  }

private:
  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  boost::shared_ptr<file_sink_t> file_sink_;
  boost::shared_ptr<console_sink_t> console_sink_;
};
