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
#include <SharedFlags.h>

void Logger::init(const std::string& appName) {
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        absl::InitializeLog();

        auto test = absl::GetFlag(FLAGS_log_dir);
        // Optionally set log dir via env or here
        if (!std::filesystem::exists(absl::GetFlag(FLAGS_log_dir))) {
            std::filesystem::create_directories(absl::GetFlag(FLAGS_log_dir));
        }

#ifdef NDEBUG
        // Release mode: only log to file (suppress stderr/stdout)
        absl::SetStderrThreshold(absl::LogSeverity::kFatal);
#else
        // Debug mode: log to stdout + file
        absl::SetStderrThreshold(absl::LogSeverity::kInfo);
#endif
        if (absl::GetFlag(FLAGS_silent)) {
            absl::SetMinLogLevel(absl::LogSeverityAtLeast::kError);
        }

        //append date and time to the front of file name in YYYYMMDD_HHMMSS format
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_r(&now_time, &tm_buf);
        char datetime[32];
        std::strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", &tm_buf);

        std::string log_file = absl::GetFlag(FLAGS_log_dir) + "/" + datetime + std::string("_") + appName + ".log";
        instance().fileLogSink_ = std::make_unique<FileLogSink>(log_file);
        absl::AddLogSink(instance().fileLogSink_.get());
    });
}
