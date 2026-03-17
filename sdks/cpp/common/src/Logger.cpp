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
 */

// common
#include <Logger.h>
#include <Config.h>

void Logger::init(const std::string& appName) {
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        absl::InitializeLog();
        
        if (!std::filesystem::exists(catena::common::config::log_dir)) {
            std::filesystem::create_directories(catena::common::config::log_dir);
        }

#ifdef NDEBUG
        // Release mode: only show LOG
        absl::SetGlobalVLogLevel(0);
#else
        // Debug mode: show both LOG and VLOG
        absl::SetGlobalVLogLevel(2);
#endif
        if (catena::common::config::silent) {
            absl::SetMinLogLevel(absl::LogSeverityAtLeast::kError);
        }
        
        absl::SetStderrThreshold(absl::LogSeverity::kInfo);

        //append date and time to the front of file name in YYYYMMDD_HHMMSS format
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_r(&now_time, &tm_buf);
        char datetime[32];
        std::strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", &tm_buf);

        std::string log_file = catena::common::config::log_dir + "/" + datetime + std::string("_") + appName + ".log";
        instance().fileLogSink_ = std::make_unique<FileLogSink>(log_file);
        absl::AddLogSink(instance().fileLogSink_.get());
    });
}

using namespace boost::log;
using namespace catena::common;
namespace expr = expressions;
typedef sinks::synchronous_sink<sinks::text_file_backend> file_sink_t;

// Helper function dictating the format of a log record
void catena_formatter(record_view const& rec, formatting_ostream &strm) {
    static std::atomic<unsigned int> rec_id{0};
    strm << (++rec_id)
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

// Helper filter for VLOG
bool catena_vlog_filter(boost::log::attribute_value_set const& attrs) {
    auto is_vlog = boost::log::extract<bool>("IsVlog", attrs);
    auto vlog_enabled = boost::log::extract<bool>("VlogEnabled", attrs);

    if (*is_vlog && !(*vlog_enabled))
        return false;
    return true;
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
void Logger2::init(const std::string& appName) {
    if (!std::filesystem::exists(config::log_dir)) {
        std::filesystem::create_directories(config::log_dir);
    }
    
    std::string fileName = "%Y%m%d_%H%M%S_" + appName + ".log";
    const int MB = 1024 * 1024;
    
    boost::shared_ptr<sinks::text_file_backend> text_backend =
    boost::make_shared<sinks::text_file_backend >(
        keywords::file_name = config::log_dir + "/" + appName + ".log",
        keywords::target_file_name = config::log_dir + "/" + fileName,
        keywords::rotation_size = config::log_size * 50
    );
    
    boost::shared_ptr<file_sink_t> sink(new file_sink_t(text_backend));
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = config::log_dir,
        keywords::max_files = config::log_count
    ));
    sink->locked_backend()->scan_for_files();
    sink->locked_backend()->auto_flush();
    sink->set_formatter(&catena_formatter);
    sink->set_filter(&catena_filter);

    boost::shared_ptr<core> core = core::get();
    core->add_sink(sink);
    add_common_attributes();  

    //Filter VLOG if not in debug
#ifdef NDEBUG
    core->add_global_attribute("VlogEnabled", attributes::constant<bool>(false));
#else
    core->add_global_attribute("VlogEnabled", attributes::constant<bool>(true));
#endif
}
