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
 * @brief Handles Connections to Peers
 * @file PeerManager.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// common
#include <patterns/Singleton.h>

// connections/gRPC
#include <PeerInfo.h>

#include <unordered_map>
#include <string>

namespace catena {
/**
 * @brief Handles connections to Peers.
 */
class PeerManager : public catena::patterns::Singleton<PeerManager> {
  public:
    /**
     * type definition for a PeerID
     */
    using PeerID = PeerInfo::PeerID;

    /**
     * As a singleton, we needn't provide a default constructor.
     * @since 1.0.0
     */
    PeerManager() = delete;

    /**
     * Pattern constructor called by Singleton::getInstance.
     * Using Protector as a dummy parameter prevents its use by
     * client code.
     * @since 1.0.0
     */
    explicit PeerManager(Protector) {}

    /**
     * @brief add a peer to the manager
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     */
    PeerInfo& addPeer(grpc::ServerContext* context, grpc::ServerWriter<catena::PushUpdates>* writer) {
        const PeerID id = std::hash<std::string>{}(context->peer());
        peers_.emplace(id, PeerInfo{context, writer}); 
        return peers_.at(id); 
    }

    /**
     * @brief remove a peer from the manager
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     */
    void removePeer(const std::string& id) { peers_.erase(std::hash<std::string>{}(id)); }

    /**
     * @brief get a peer by id
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     * @return PeerInfo&
     */
    PeerInfo& getPeer(const std::string& id) { return peers_.at(std::hash<std::string>{}(id)); }

    /**
     * @brief get a peer by id
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     * @return const PeerInfo&
     */
    const PeerInfo& getPeer(const std::string& id) const { return peers_.at(std::hash<std::string>{}(id)); }

    /**
     * @brief check if a peer is in the manager
     *
     * @param id the result of grpc::ServerContext::peer(), aka std::string.
     * @return true if the peer is in the manager
     */
    bool hasPeer(const std::string& id) const { return peers_.contains(std::hash<std::string>{}(id)); }

    /**
     * @brief handle updates from the service
     */
    void handleValueUpdate(const ParamAccessor& p, ParamIndex idx) {
        for (auto& [id, peer] : peers_) {
            peer.handleValueUpdate(p, idx);
        }
    }

    /**
     * @brief handle updates from other clients
     */
    void handleValueUpdate(const ParamAccessor& p, ParamIndex idx, const std::string& peer) {
        PeerID ignore = std::hash<std::string>{}(peer);
        for (auto& [id, peer] : peers_) {
            if (id != ignore) {
                peer.handleValueUpdate(p, idx);
            }
        }
    }

  private:
    std::unordered_map<PeerID, PeerInfo> peers_;
};
}  // namespace catena