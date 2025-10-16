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
 * @file IHeartbeat.h
 * @brief Implements a Heartbeat object.
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @copyright Copyright (c) 2025 Ross Video
 */

#pragma once

#include "IHeartbeat.h"

// std
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace catena {
namespace common {

/*
 * @brief Implements a Heartbeat object.
 */
class Heartbeat : public IHeartbeat {
  public:
    Heartbeat() {}
    ~Heartbeat();

    void start(int32_t milliseconds) override;
    void stop() override;
    vdk::signal<void()>& getHeartbeatSignal() override { return signal_; }

  private:
    vdk::signal<void()> signal_;
    bool running_{false};
    std::mutex mutex_;
    std::condition_variable condition_;
    std::unique_ptr<std::thread> thread_;
};
}  // namespace common
}  // namespace catena