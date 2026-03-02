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
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>

void FileLogSink::Send(const absl::LogEntry& entry) {
    // check if we need to rotate logs before writing
    const std::string_view msg = entry.text_message_with_prefix_and_newline();
    const size_t msg_size = msg.size();
    const auto current_pos = log_file_.tellp();
    LOG(INFO) << "Current log file position: " << current_pos << ", message size: " << msg_size << ", log file size limit: " << log_file_size_;
    if (log_file_.is_open() && msg.size() + log_file_.tellp() > log_file_size_) {
        rotateLogs();
    }
    if (log_file_.is_open()) {
        log_file_ << msg;
        log_file_.flush();  // Ensure immediate write to file
    }
}

void FileLogSink::rotateLogs() {
    if (log_file_.is_open()) {
        log_file_.close();
        // count the number of existing log files, and remove the oldest if we have reached the max count
        std::set<std::filesystem::path> existing_logs;
        for (const auto& entry : std::filesystem::directory_iterator(log_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                existing_logs.insert(entry.path());
            }
        }
        if (existing_logs.size() >= log_file_count_) {
            std::filesystem::remove(*existing_logs.begin());
        }
    }
    // create a new log file with a timestamp in the name
    //append date and time to the front of file name in YYYYMMDD_HHMMSS format
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_r(&now_time, &tm_buf);
    char datetime[32];
    std::strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", &tm_buf);

    std::string log_file = log_dir_ + "/" + datetime + std::string("_") + appName_ + ".log";
    log_file_.open(log_file, std::ios_base::app);
}

void Logger::init(const std::string& appName) {
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        absl::InitializeLog();
        
        boost::shared_ptr<boost::log::sinks::text_file_backend> backend = boost::make_shared<boost::log::sinks::text_file_backend>(
            boost::log::keywords::file_name = catena::common::config::log_dir + "/%Y%m%d_%H%M%S_" + appName + ".log",
            boost::log::keywords::rotation_size = catena::common::config::log_file_size,
            boost::log::keywords::max_files = catena::common::config::log_file_count,
            boost::log::keywords::auto_flush = true
        );

        boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> sink(backend);

        if (catena::common::config::log_file && !std::filesystem::exists(catena::common::config::log_dir)) {
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
        } else {
            std::string log_level = catena::common::config::log_level;
            std::transform(log_level.begin(), log_level.end(), log_level.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (log_level == "warning") {
                absl::SetMinLogLevel(absl::LogSeverityAtLeast::kWarning);
            } else if (log_level == "error") {
                absl::SetMinLogLevel(absl::LogSeverityAtLeast::kError);
            } else if (log_level == "info") {
                absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
            } else {
                absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
                LOG(WARNING) << "Invalid log level specified: " << catena::common::config::log_level << ". Defaulting to INFO.";
            }
        }

        if (catena::common::config::log_file) {
            instance().fileLogSink_ = std::make_unique<FileLogSink>(appName, catena::common::config::log_dir,
                                                                    catena::common::config::log_file_size,
                                                                    catena::common::config::log_file_count);
            absl::AddLogSink(instance().fileLogSink_.get());
        }
    });
}
