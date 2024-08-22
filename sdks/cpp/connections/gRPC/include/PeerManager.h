#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @brief Handles Connections to Peers
 * @file PeerManager.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// common
#include <patterns/Singleton.h>

// connections/gRPC
#include <PeerInfo.h>

#include <unordered_map>
#include <string>

namespace catena {
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