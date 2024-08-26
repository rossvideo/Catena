#pragma once

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

/**
 * @file IConstraint.h
 * @brief Interface for constraints
 * @author john.naylor@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include "google/protobuf/message_lite.h" 

namespace catena {
namespace common {

/**
 * @brief Interface for constraints
 */
class IConstraint {
public:
    IConstraint() : oid_{}, isShared_{} {}
    IConstraint(std::string oid, bool shared) : oid_{oid}, isShared_{shared} {}
    virtual ~IConstraint() = default;

    /**
     * @brief serialize the constraint to a protobuf message
     * @param constraint the protobuf message to populate, NB, implementations should 
     * dynamically cast this to catena::Constraint
     */
    virtual void toProto(google::protobuf::MessageLite& constraint) const = 0;

    /**
     * @brief applies constraint to src and writes constrained value to dst
     * @param src a catena::Value to apply the constraint to
     */
    virtual void apply(void* src) const = 0;

    /**
     * @brief check if the constraint is shared
     * @return true if the constraint is shared, false otherwise
     */
    inline bool getShared() const { return isShared_; }
    
    /**
     * @brief set the shared flag
     * @param shared true if the constraint is shared, false otherwise
     */
    void setShared(bool shared) { isShared_ = shared; }

    /**
     * @brief get the oid of the constraint
     * @return the oid of the constraint
     */
    inline const std::string& getOid() const { return oid_; }

    /**
     * @brief set the oid of the constraint
     */
    void setOid(const std::string& oid) { oid_ = oid; }

protected:
    std::string oid_; ///< the oid of the constraint
    bool isShared_;   ///< true if the constraint is shared, false otherwise
};

} // namespace common
} // namespace catena