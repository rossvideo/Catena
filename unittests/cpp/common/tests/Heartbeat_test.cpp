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

/**
 * @brief This file is for testing the Heartbeat.cpp file.
 * @author andrew.brown@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <rpc/Heartbeat.h>
#include <Logger.h>

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

using namespace catena::common;

class HeartbeatTest : public ::testing::Test {
  protected:
    Heartbeat hb;
    std::atomic<int32_t> count{0};
    
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("HeartbeatTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    void SetUp() override {
        hb.getHeartbeatSignal().connect([this]() { count++; });
    }

    void TearDown() override { hb.stop(); }
};

/*
 * Test that the heartbeat signals at the expected interval.
 */
TEST_F(HeartbeatTest, SignalsAtInterval) {
    hb.start(100);  // 100 ms interval
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    hb.stop();
    // Should have signaled about 3 times
    EXPECT_GE(count.load(), 3);
    EXPECT_LE(count.load(), 4);  // Allow for timing variance
}

/*
 * Test that the heartbeat stops when requested.
 */
TEST_F(HeartbeatTest, StopsWhenRequested) {
    hb.start(100);  // Long interval
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    hb.stop();
    int32_t before = count.load();
    EXPECT_GE(before, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Should not signal after stop
    EXPECT_EQ(count.load(), before);
}

/*
 * Test that the heartbeat does not signal if stopped immediately
 */
TEST_F(HeartbeatTest, StopsImmediately) {
    hb.start(500);  // Long interval
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    hb.stop();
    EXPECT_EQ(count.load(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // Should not signal after stop
    EXPECT_EQ(count.load(), 0);
}

TEST_F(HeartbeatTest, StartDoesNotDoubleStart) {
    hb.start(100);
    hb.start(100);  // Should not start a second thread
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    hb.stop();
    // so it won't count up very fast
    EXPECT_GE(count.load(), 2);
    EXPECT_LE(count.load(), 3);  // Allow for timing variance
}

TEST_F(HeartbeatTest, MultipleStops) {
    hb.start(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    hb.stop();
    // Call stop again, should not crash or hang
    hb.stop();
    SUCCEED();
}

TEST_F(HeartbeatTest, NoSlotsAttachedToSignal) {
    // make a new one with no slots
    Heartbeat hb;
    hb.start(100);  // Start with no slots connected
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    hb.stop();
    SUCCEED();
}

TEST_F(HeartbeatTest, ExceptionInSlot) {
    hb.getHeartbeatSignal().connect([]() { throw std::runtime_error("Test exception"); });
    hb.start(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    hb.stop();
    // the count should still go up
    EXPECT_GE(count.load(), 3);
    EXPECT_LE(count.load(), 4);
}

TEST_F(HeartbeatTest, NonStdExceptionInSlot) {
    hb.getHeartbeatSignal().connect([]() { throw 42; });
    hb.start(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    hb.stop();
    // the count should still go up
    EXPECT_GE(count.load(), 3);
    EXPECT_LE(count.load(), 4);
}

TEST_F(HeartbeatTest, CanStopFromSlot) {
    hb.getHeartbeatSignal().connect([this]() { hb.stop(); });
    hb.start(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    // should have stopped after the first signal
    EXPECT_EQ(count.load(), 1);
}
