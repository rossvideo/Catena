#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @brief Holds information about connected Peers
 * @file PeerInfo.h
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

// common
#include <IParam.h>

// gRPC interface
#include <interface/service.grpc.pb.h>

#include <string>

namespace catena {
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