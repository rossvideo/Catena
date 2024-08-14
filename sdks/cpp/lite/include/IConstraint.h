#pragma once

/**
 * @file IConstraint.h
 * @brief Interface for constraints
 * @author john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

namespace catena {

class Constraint; // forward declaration


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
    virtual bool isValid() = 0;

    /**
     * @brief constrain the underlying parameter value to lie within the constraint.
     */
    virtual void constrain() = 0;

    /**
     * @brief serialize the constraint to a protobuf message
     * @param constraint the protobuf message to populate
     */
    void toProto(catena::Constraint& constraint) const;
};

} // namespace lite
} // namespace catena