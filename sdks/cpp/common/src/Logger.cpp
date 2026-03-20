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

// common
#include <Logger.h>
#include <Config.h>

// boost
#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/support/date_time.hpp>

using namespace boost::log;
using namespace catena::common;
namespace expr = expressions;

// Helper function dictating the format of a log record
void catena_formatter(record_view const& rec, formatting_ostream &strm) {
    strm << extract<unsigned int>("RecordID", rec)
        << ": " << rec[trivial::severity];
    
    // Format timestamp asd HH::MM::SS.Microseconds
    std::ostringstream oss;
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
    facet->format("%H:%M:%S.%f");
    auto ts = extract<boost::posix_time::ptime>("TimeStamp", rec);
    oss.imbue(std::locale(oss.getloc(), facet));
    oss << *ts;

    strm << " " << oss.str()
        << "  " << extract<int>("KernelThreadID", rec)
        << " " << extract<std::string>("File", rec)
        << ":" << extract<int>("Line", rec)
        <<"]  " << rec[expr::message];
}

// Helper for log_count=1, ensures only active file is in directory
void clear_directory(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::filesystem::remove_all(entry.path());
    }
}

// Helper filter for VLOG
bool catena_vlog_filter(boost::log::attribute_value_set const& attrs) {
    auto verbosity_level = attrs["Verbosity"].extract<int>();
    if (!verbosity_level)
        return true;
    return verbosity_level.get() <= config::log_verbosity;
}

// Helper filter for severity level
bool catena_severity_filter(boost::log::attribute_value_set const& attrs) {
    trivial::severity_level filter_level = trivial::info;
    if (config::log_level.compare("WARNING") == 0) {
        filter_level = trivial::warning;
    } else if (config::log_level.compare("ERROR") == 0) {
        filter_level = trivial::error;
    }
    return attrs[trivial::severity] >= filter_level;
}

// Main filter that calls the helpers
bool catena_filter(boost::log::attribute_value_set const& attrs) {
    if (config::silent) {
        return false;
    }
    return catena_vlog_filter(attrs) && catena_severity_filter(attrs);
}

/**
 *  Initializes the logging system
 *  @param appName The name of the application. Will be used to determine the log file name.
 */
void Logger::init(const std::string& appName) {
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        if (!std::filesystem::exists(config::log_dir)) {
            std::filesystem::create_directories(config::log_dir);
        }
        
        std::string fileName = config::log_dir + "/" + "%Y%m%d_%H%M%S_" + appName + ".log";
        const int MB = 1024 * 1024;
        
        boost::shared_ptr<core> core = core::get();
        add_common_attributes();  // Timestamps
        if (config::log_file) {
            // Create file sink and set parameters
            instance().file_sink_ = boost::make_shared<file_sink_t>(
                keywords::file_name = fileName,
                keywords::rotation_size = config::log_size * MB
            );
            auto& file_sink = instance().file_sink_;
            // Set filtering, formatting, and file collecting
            if (config::log_count > 1) {
                file_sink->locked_backend()->set_file_collector(sinks::file::make_collector(
                    keywords::target = config::log_dir,
                    keywords::max_files = config::log_count - 1, // The backend doesn't count the active file, so adjust down by 1
                    keywords::enable_final_rotation = false
                ));
                file_sink->locked_backend()->scan_for_files(sinks::file::scan_all);
            } else {
                file_sink->locked_backend()->set_target_file_name_pattern(fileName);
                file_sink->locked_backend()->set_file_collector(nullptr);
                clear_directory(config::log_dir);
            }
            file_sink->locked_backend()->auto_flush();
            file_sink->set_formatter(&catena_formatter);
            file_sink->set_filter(&catena_filter);
            core->add_sink(file_sink);
            // Force a rotation on startup to ensure old files are deleted
            BOOST_LOG_TRIVIAL(info);
            instance().file_sink_->locked_backend()->rotate_file();
        }
        if (config::log_console) {
            // Create console sink and set filtering, formatting, file collecting
            auto& console_sink = instance().console_sink_;
            console_sink = boost::make_shared<console_sink_t>();
            console_sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, boost::null_deleter()));
            console_sink->locked_backend()->auto_flush();
            console_sink->set_formatter(&catena_formatter);
            console_sink->set_filter(&catena_filter);
            core->add_sink(console_sink);
        }
    });
}
