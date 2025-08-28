#pragma once

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
 * @file IConstraint.h
 * @brief Interface for constraints
 * @author john.naylor@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

// protobuf interface
#include "interface/param.pb.h"
#include "interface/constraint.pb.h" 

namespace catena {
namespace common {

/**
 * @brief Interface for constraints
 */
class IConstraint {
public:
    IConstraint() = default;
    virtual ~IConstraint() = default;

    /**
     * @brief Serializes the constraint to a protobuf message.
     * @param constraint The protobuf message to serialize to.
     */
    virtual void toProto(st2138::Constraint& constraint) const = 0;

    /**
     * @brief Checks if the constraint is satisfied by src.
     * @param src A catena::Value to check the constraint against.
     * @return True if the constraint is satisfied, false otherwise.
     */
    virtual bool satisfied(const st2138::Value& src) const = 0;

    /**
     * @brief Applies constraint to src and returns the constrained value.
     * @param src A catena::Value to apply the constraint to.
     * @return A catena::Value with the constraint applied.
     */
    virtual st2138::Value apply(const st2138::Value& src) const = 0;

    /**
     * @brief Checks if the constraint is a range constraint.
     * @return True if the constraint is a range constraint, False otherwise.
     */
    virtual bool isRange() const = 0;

    /**
     * @brief Checks if the constraint is a shared constraint.
     * @return True if the constraint is shared, False otherwise.
     */
    virtual bool isShared() const = 0;

    /**
     * @brief Gets the constraint oid.
     * @return The oid of the constraint.
     */
    virtual const std::string& getOid() const = 0;
};

} // namespace common
} // namespace catena
