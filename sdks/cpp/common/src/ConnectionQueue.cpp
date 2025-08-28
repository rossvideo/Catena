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

// common
#include <rpc/ConnectionQueue.h>
#include <Logger.h>

using catena::common::ConnectionQueue;
using catena::common::IConnect;

bool ConnectionQueue::registerConnection(IConnect* cd) {
    bool added = false;
    if (!cd) {
        throw catena::exception_with_status("Cannot add nullptr to connection queue", catena::StatusCode::INVALID_ARGUMENT);
    } else {
        std::lock_guard<std::mutex> lock(mtx_);
        // Start by clearing out the connectionQueue_ of any cancelled connections.
        if (connectionQueue_.size() >= maxConnections_) {
            std::erase_if(connectionQueue_,
                // Calls shutdown and returns true if the connection is cancelled.
                [](IConnect* connection) {
                    bool cancelled = connection->isCancelled();
                    if (cancelled) { connection->shutdown(); }
                    return cancelled;
                });
        }
        // Find the index to insert the new connection based on priority.
        auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
            [cd](const IConnect* connection) {
                assert(connection);
                return *cd < *connection;
            });
        // Based on the iterator, determine if we can add the connection.
        if (connectionQueue_.size() >= maxConnections_) {
            if (it != connectionQueue_.begin()) {
                // Forcefully shutting down lowest priority connection.
                connectionQueue_.insert(it, cd);
                connectionQueue_.front()->shutdown();
                connectionQueue_.erase(connectionQueue_.begin());
                added = true;
            }
        } else {
            connectionQueue_.insert(it, cd);
            added = true;
        }
    }
    return added;
}

void ConnectionQueue::deregisterConnection(const IConnect* cd) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::find_if(connectionQueue_.begin(), connectionQueue_.end(),
                           [cd](const IConnect* i) { return i == cd; });
    if (it != connectionQueue_.end()) {
        connectionQueue_.erase(it);
    }
    DEBUG_LOG << "Connected users remaining: " << connectionQueue_.size() << '\n';
}
