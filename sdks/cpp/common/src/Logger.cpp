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
typedef sinks::synchronous_sink<sinks::text_file_backend> sink_t;

void init_file_collecting(boost::shared_ptr<sink_t> sink)
{
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = config::log_dir,
        keywords::max_files = config::log_count
    ));
}

void formatter(record_view const& rec, formatting_ostream &strm) {
    namespace expr = expressions;

    strm << extract<unsigned int>("LineID", rec)
        << ": " << rec[trivial::severity];


    expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f");
}

void Logger2::init(const std::string& appName) {
    if (!std::filesystem::exists(config::log_dir)) {
        std::filesystem::create_directories(config::log_dir);
    }

    std::string fileName = "%Y%m%d_%H%M%S_" + appName + ".log";
    const int MB = 1024 * 1024;
    
    boost::shared_ptr<sinks::text_file_backend> backend =
    boost::make_shared<sinks::text_file_backend >(
        keywords::file_name = config::log_dir + "/" + appName + ".log",
        keywords::target_file_name = config::log_dir + "/" + fileName,
        keywords::rotation_size = config::log_size * 50
    );
    
    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    boost::shared_ptr<sink_t> sink(new sink_t(backend));
    init_file_collecting(sink);
    sink->locked_backend()->scan_for_files();
    sink->locked_backend()->auto_flush();

    boost::shared_ptr<core> core = core::get();
    core->add_sink(sink);

    add_common_attributes();
    

    sink->set_formatter(expressions::stream 
        << expressions::attr<unsigned int>("LineID")
        << ": " << trivial::severity
        << " " << expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
        << "  " << expressions::attr<attributes::current_process_id::value_type>("ProcessID")
        << "  " << expressions::attr<attributes::current_thread_id::value_type>("ThreadID")
        << " ??:??]  "
        << expressions::message);
}
