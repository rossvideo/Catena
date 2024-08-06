#pragma once

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
    virtual ~IConstraint() = default;

    /**
     * @brief serialize the constraint to a protobuf message
     * @param constraint the protobuf message to populate, NB, implementations should 
     * dynamically cast this to catena::Constraint
     */
    virtual void toProto(google::protobuf::MessageLite& constraint) const = 0;

    /**
     * @brief applies constraint to src and writes constrained value to dst
     * @param src a catena::Value containing the value to apply the constraint to
     * @param dst a catena::Value to write the constrained value to
     */
    virtual void apply(void* dst, const void* src) const = 0;

    /**
     * @brief check if the constraint is shared
     * @return true if the constraint is shared, false otherwise
     */
    bool isShared() const { return isShared_; }
    
    /**
     * @brief set the shared flag
     * @param shared true if the constraint is shared, false otherwise
     */
    void setShared(bool shared) { isShared_ = shared; }

    /**
     * @brief get the oid of the constraint
     * @return the oid of the constraint
     */
    const std::string& getOid() const { return oid_; }

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