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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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

#include <rpc/Heartbeat.h>
#include <Logger.h>

using catena::common::Heartbeat;

Heartbeat::~Heartbeat() {
    // make sure the heartbeat is stopped
    stop();
}

void Heartbeat::start(int32_t milliseconds) {
    // acquire the lock
    std::lock_guard<std::mutex> lock(mutex_);
    // don't allow starting if already running
    if (running_) {
        return;
    }
    // create the thread
    running_ = true;
    thread_ = std::make_unique<std::thread>([this, milliseconds]() {
        // the thread acquires the lock
        std::unique_lock<std::mutex> lock(mutex_);
        while (running_) {
            // this is basically an interruptible sleep
            condition_.wait_for(lock, std::chrono::milliseconds(milliseconds));
            // don't emit if we're shutting down
            if (running_) {
                // unlock while we emit the signal
                lock.unlock();
                try {
                    // emit the signal
                    signal_.emit();
                } catch (const std::exception& e) {
                    // log exceptions
                    DEBUG_LOG << "Exception in heartbeat slot: " << e.what();
                } catch (...) {
                    // log non-std exceptions
                    DEBUG_LOG << "Unknown exception in heartbeat slot";
                }
                // re-lock before the next iteration
                lock.lock();
            }
        }
    });
}

void Heartbeat::stop() {
    {
        // lock inside a scope so that the mutex is released automatically
        // and the lock allows the thread to join
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        condition_.notify_all();
    }
    // wait for thread to finish
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}
