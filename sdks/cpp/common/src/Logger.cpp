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
 * @file Logger.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-03-20
 */
#include <filesystem>
#include <regex>
#include <cstring>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// common
#include <Logger.h>
#include <Config.h>

// boost
#include <boost/filesystem/operations.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/support/date_time.hpp>

using namespace boost::log;
using namespace catena::common;
namespace expr = expressions;
namespace sinks_file = boost::log::sinks::file;

// Helper for LOG() macro to get basename of __FILE__
const char* LogHelper::log_basename(const char* path) {
    const char* p = std::strrchr(path, '/');
    if (!p) p = std::strrchr(path, '\\');
    return p ? p + 1 : path;
}

// Helper for LOG() macro to get kernel id of thread
int LogHelper::kernel_thread_id() {
    #ifdef _WIN32
    return static_cast<int>(GetCurrentThreadID()); // Needs testing on Windows build
    #else
    return static_cast<int>(gettid());
    #endif
}

// Helper to initialize logging for unit tests
void LogHelper::set_up_test_logs(std::string directory, std::string appName) {
    config::log_level = "TRACE"; 
    config::log_console = true;
    config::log_file = true;
    config::log_dir = directory;
    config::log_size = 10;
    config::log_count = 3; // 1 active + 2 archived, final rotation is true so only 2 will remain after teardown 
    config::log_final_rotation = true;
    Logger::init(appName);
}

// Helper for log_count == 1: Active file only.
class single_file_collector final : public sinks_file::collector {
public:
    single_file_collector(std::filesystem::path log_dir, std::string app_name)
        : log_dir_(std::move(log_dir)), app_name_(std::move(app_name)) {}

    // Deletes rotated file
    void store_file(boost::filesystem::path const& src_path) override {
        boost::filesystem::remove(src_path);
    }

    // Needs implementation, not used
    bool is_in_storage(boost::filesystem::path const&) const override {
        return false;
    }

    // Used on startup to delete old archived files matching the app
    sinks_file::scan_result scan_for_files( sinks_file::scan_method method,
        boost::filesystem::path const& pattern = boost::filesystem::path()) override {
        (void)pattern;
        sinks_file::scan_result result;

        std::regex const log_regex(R"(^\d{8}_\d{6}_)" + app_name_ + R"(\.log$)");
        for (const auto& entry : std::filesystem::directory_iterator(log_dir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::string const filename = entry.path().filename().string();
            if (std::regex_match(filename, log_regex)) {
                std::filesystem::remove(entry.path());
                ++result.found_count;
            }
        }
        return result;
    }

private:
    std::filesystem::path log_dir_;
    std::string app_name_;
};

// Helper function dictating the format of a log record
void catena_formatter(record_view const& rec, formatting_ostream &strm) {
    strm << rec[trivial::severity];
    
    // Format timestamp as HH::MM::SS.Microseconds
    std::ostringstream oss;
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
    facet->format("%H:%M:%S.%f");
    auto ts = extract<boost::posix_time::ptime>("TimeStamp", rec);
    oss.imbue(std::locale(oss.getloc(), facet));
    oss << *ts;

    strm << " " << oss.str()
        << "  " << rec[LogHelper::KernelThreadID]
        << " " << rec[LogHelper::File]
        << ":" << rec[LogHelper::Line]
        <<"]  " << rec[expr::message];
}

// Helper for filter
static trivial::severity_level filter_level = trivial::trace;
void set_filter_level() {
    if (config::log_level.compare("TRACE") == 0) {
        filter_level = trivial::trace;
    } else if (config::log_level.compare("DEBUG") == 0) {
        filter_level = trivial::debug;
    } else if (config::log_level.compare("INFO") == 0) {
        filter_level = trivial::info;
    } else if (config::log_level.compare("WARNING") == 0) {
        filter_level = trivial::warning;
    } else if (config::log_level.compare("ERROR") == 0) {
        filter_level = trivial::error;
    } else if (config::log_level.compare("FATAL") == 0) {
        filter_level = trivial::fatal;
    }
}

// Main filter that calls the helpers
bool catena_filter(boost::log::attribute_value_set const& attrs) {
    if (config::silent) {
        return false;
    }
    return attrs[trivial::severity] >= filter_level;
}

//Initialize static member variables
bool Logger::initialized_ = false;
std::mutex Logger::mutex_;

/**
 *  Initializes the logging system
 *  @param appName The name of the application. Will be used to determine the log file name.
 */
void Logger::init(const std::string& appName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        if (!std::filesystem::exists(config::log_dir)) {
            std::filesystem::create_directories(config::log_dir);
        }
        
        std::string activeName = config::log_dir + "/" + appName + ".log";
        std::string targetName = config::log_dir + "/" + "%Y%m%d_%H%M%S_" + appName + ".log";
        const int MB = 1024 * 1024;
        
        boost::shared_ptr<core> core = core::get();
        add_common_attributes();  // Timestamps
        set_filter_level();
        if (config::log_file) {
            // Create file sink and set parameters
            instance().file_sink_ = boost::make_shared<file_sink_t>(
                keywords::file_name = activeName,
                keywords::target_file_name = targetName,
                keywords::rotation_size = config::log_size * MB,
                keywords::enable_final_rotation = config::log_final_rotation,
                keywords::open_mode = std::ios_base::out | (config::log_append? std::ios_base::app : std::ios_base::trunc)
            );
            auto& file_sink = instance().file_sink_;

            if (config::log_count > 1) {
                // Multi file (1 Active + x Archived)
                file_sink->locked_backend()->set_file_collector(sinks::file::make_collector(
                    keywords::target = config::log_dir,
                    keywords::max_files = config::log_count - 1 // Amount of archived files, collector doesn't recognize active files
                ));
            } else {
                // Single file (Active only)
                file_sink->locked_backend()->set_file_collector(boost::make_shared<single_file_collector>(
                    std::filesystem::path(config::log_dir), appName));
            }
            
            // Set filtering and formatting
            file_sink->locked_backend()->auto_flush();
            file_sink->set_formatter(&catena_formatter);
            file_sink->set_filter(&catena_filter);
            core->add_sink(file_sink);
            
            // Force a rotation on startup to ensure old active file is rotated. Only necessary for multi, single's scan handles this.
            file_sink->locked_backend()->scan_for_files(sinks::file::scan_matching);
            if (std::filesystem::is_regular_file(config::log_dir + "/" + appName + ".log") && config::log_count > 1) {
                // Swap to append to rotate old file, swap back if log_append=false
                instance().file_sink_->locked_backend()->set_open_mode(std::ios_base::out | std::ios_base::app);
                BOOST_LOG_TRIVIAL(info) << "Startup rotation";
                instance().file_sink_->locked_backend()->rotate_file();
                if (!config::log_append) {
                    instance().file_sink_->locked_backend()->set_open_mode(std::ios_base::out | std::ios_base::trunc);
                }
            }
        }
        if (config::log_console) {
            // Create console sink and set filtering and formatting
            auto& console_sink = instance().console_sink_;
            console_sink = boost::make_shared<console_sink_t>();
            console_sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, boost::null_deleter()));
            console_sink->locked_backend()->auto_flush();
            console_sink->set_formatter(&catena_formatter);
            console_sink->set_filter(&catena_filter);
            core->add_sink(console_sink);
        }
        initialized_ = true;
    }
}
