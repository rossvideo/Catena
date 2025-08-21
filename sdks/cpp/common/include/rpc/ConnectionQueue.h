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
 * @file ConnectionQueue.h
 * @brief Implements the ConnectionQueue class which manages connections to a
 * service.
 * @author Ben Whitten (benjamin.whitten@rossvideo.com)
 * @copyright Copyright (c) 2025 Ross Video
 */

#pragma once

// common
#include "IConnectionQueue.h"

// std
#include <vector>
#include <mutex>

namespace catena {
namespace common {

/**
 * @brief Class for managing connections to a service.
 * 
 * This class manages connections via a priority queue which will forcefully
 * disconnect lower priority connections which exceed the defined max.
 * Priority is determined by the IConnect object.
 */
class ConnectionQueue : public IConnectionQueue {
  public:
    /**
     * @brief Constructor.
     * @param maxConnections The maximum number of connections allowed in the
     * queue.
     */
    ConnectionQueue(uint32_t maxConnections) : maxConnections_(maxConnections) {}
    /**
     * @brief Regesters a Connect CallData object into the priority queue.
     * 
     * If the queue is full, the queue will first check for any cancelled
     * connections it can remove. If none are found, it will then forcefully
     * shutdown the lowest priority connection to make way for the new one.
     * 
     * @param cd The Connect CallData object to register.
     * @return True if successfully registered, False otherwise.
     */
    bool registerConnection(IConnect* cd) override;
    /**
     * @brief Deregisters a Connect CallData object from the priority queue if
     * it hasn't aleady.
     * 
     * @param cd The Connect CallData object to deregister.
     */
    void deregisterConnection(const IConnect* cd) override;

  protected:
    /**
     * @brief The maximum number of connections allowed in the queue.
     */
    uint32_t maxConnections_;
    /**
     * @brief Mutex to protect calls to register and deregisterConnection
     * across multiple threads.
     */
    std::mutex mtx_;
    /**
     * @brief The priority queue for Connect CallData objects.
     * 
     * This is a vector instead of an actual priority queue object as
     * individual access is required for deregistering connections at the end
     * of their lifetime.
     */
    std::vector<IConnect*> connectionQueue_;
};

} // namespace common
} // namespace catena
