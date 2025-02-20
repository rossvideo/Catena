#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

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

/**
 * @brief Holds information about connected Peers
 * @file PeerInfo.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// common
#include <IParam.h>

// gRPC interface
#include <interface/service.grpc.pb.h>

#include <string>

namespace catena {
/**
 * @brief Holds information about connected Peers.
 */
class PeerInfo {
  public:
    /**
     * type definition for a PeerID
     */
    using PeerID = std::size_t;

    /**
     * type definition for a Writer to the peer
     */
    using Writer = grpc::ServerWriter<catena::PushUpdates>;

    /**
     * @brief Construct a new Peer Info object
     *
     * Calculates a hash of the id string which is stored as the peer id.
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     */
    explicit PeerInfo(grpc::ServerContext* ctx, grpc::ServerWriter<catena::PushUpdates>* writer)
        : context_{ctx}, writer_{writer} {}

    /**
     * PeerInfo has no default constructor
     */
    PeerInfo() = delete;

    /**
     * PeerInfo has no copy constructor
     */
    PeerInfo(const PeerInfo&) = delete;

    /**
     * PeerInfo has no copy assignment
     */
    PeerInfo& operator=(const PeerInfo&) = delete;

    /**
     * PeerInfo has move semantics
     */
    PeerInfo(PeerInfo&&) = default;

    /**
     * PeerInfo has move semantics
     */
    PeerInfo& operator=(PeerInfo&&) = default;

    /**
     * @brief returns true if the peer has a writer
     */
    bool inline has_writer() const { return writer_ != nullptr; }

    /**
     * @brief handles updates from the owning service
     */
    void handleValueUpdate(const catena::common::IParam& param/*, catena::lite::ParamIndex idx*/);

    /**
     * @brief handles the connection with the pier
     */
    grpc::Status handleConnection();

  private:
    grpc::ServerContext* context_; /**< the peer's connection context*/
    Writer* writer_;               /**< the peer's writer, could be nullptr if one hasn't been connected */
};
}  // namespace catena