#pragma once

/**
 * @file IConstraint.h
 * @brief Interface for constraints
 * @author john.naylor@rossvideo.com
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

    virtual ~IConstraint() = default;

    /**
     * @brief check if the underlying parameter value lies within the constraint.
     * @return true if the constraint is valid, false otherwise
     */
    virtual bool isValid() const = 0;

    /**
     * @brief constrain the underlying parameter value to lie within the constraint.
     */
    virtual void constrain() = 0;

    /**
     * @brief serialize the constraint to a protobuf message
     * @param constraint the protobuf message to populate, NB, implementations should 
     * dynamically cast this to catena::Constraint
     */
    virtual void toProto(google::protobuf::MessageLite& constraint) const = 0;
};

} // namespace common
} // namespace catena