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
 * @file RangeConstraint.h
 * @brief A constraint that checks if a value is within a range
 * @author john.naylor@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IConstraint.h>
#include <IParam.h>
#include <Tags.h>
#include <IDevice.h>

// catena interface
#include <google/protobuf/message_lite.h>

#include <cmath>

namespace catena {
namespace common {

/**
 * @brief Range constraint, ensures a value is within a range
 * @tparam T int or float
 */
template <typename T>
class RangeConstraint : public catena::common::IConstraint {
public:
    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param dm the device to add the constraint to
     */
    RangeConstraint(T min, T max, T step, std::string oid, bool shared, IDevice& dm)
        : min_(min), max_(max), step_{step}, 
        display_min_{min}, display_max_{max}, oid_{oid}, shared_{shared} {
        dm.addItem(oid, this);
    }

    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param parent the param to add the constraint to
     */
    template <typename U>
    RangeConstraint(T min, T max, T step, std::string oid, bool shared)
        : min_(min), max_(max), step_{step}, 
        display_min_{min}, display_max_{max}, oid_{oid}, shared_{shared} {}

    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param step the step size
     * @param display_min the minimum value to display
     * @param display_max the maximum value to display
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param dm the device to add the constraint to
     */
    RangeConstraint(T min, T max, T step, T display_min, T display_max, std::string oid,
        bool shared, IDevice& dm)
        : min_(min), max_(max), step_(step),
        display_min_{display_min}, display_max_{display_max}, oid_{oid}, shared_{shared} {
        dm.addItem(oid, this);
    }

    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param step the step size
     * @param display_min the minimum value to display
     * @param display_max the maximum value to display
     * @param oid the oid of the constraint
     * @param shared is the constraint shared
     * @param parent the param to add the constraint to
     */
    RangeConstraint(T min, T max, T step, T display_min, T display_max, std::string oid, 
        bool shared)
        : min_(min), max_(max), step_(step),
        display_min_{display_min}, display_max_{display_max}, oid_{oid}, shared_{shared} {}

    /**
     * @brief default destructor
     */
    virtual ~RangeConstraint() = default;

    /**
     * @brief checks if the value satisfies the constraint
     * @param src the value to check
     * @return true if the value satisfies the constraint
     * 
     * Checks if the value is within the range. 
     * 
     * If the step size is not 0, the value must also be a multiple of the step size.
     */
    bool satisfied(const catena::Value& src) const override {

        if constexpr(std::is_same<T, int32_t>::value) {
            return src.int32_value() >= min_ 
                && src.int32_value() <= max_
                && (!step_ || (src.int32_value() - min_) % step_ == 0);
        }

        if constexpr(std::is_same<T, float>::value) {
            return src.float32_value() >= min_ 
                && src.float32_value() <= max_
                && (!step_ || std::fmod(src.float32_value() - min_, step_) == 0);
        }
    }

    /**
     * @brief applies range constraint to a catena::Value
     * @param src a catena::Value to apply the constraint to
     * @return a catena::Value with the constraint applied
     * 
     * If the value is outside the range, the value will be constrained
     * to be within the range. If the step size is not 0, the value will
     * be constrained to be a multiple of the step size.
     */
    catena::Value apply(const catena::Value& src) const override {
        catena::Value constrainedVal;

        if constexpr(std::is_same<T, int32_t>::value) {
            // return empty value if src is not valid
            if (!src.has_int32_value()) { return constrainedVal; }
            int32_t s = src.int32_value();

            // constrain if not within allowed range
            if (s < min_) {
                constrainedVal.set_int32_value(min_);
            } else if (s > max_) {
                constrainedVal.set_int32_value(max_);
            } else if (!step_ || (s - min_) % step_ != 0) {
                constrainedVal.set_int32_value(s - ((s - min_) % step_));
            }
            return constrainedVal;
        }

        if constexpr(std::is_same<T, float>::value) {

            // return empty value if src is not valid
            if (!src.has_float32_value()) { return constrainedVal; }
            float s = src.float32_value();

            // constrain if not within allowed range
            if (s < min_) {
                constrainedVal.set_float32_value(min_);
            } else if (s > max_) {
                constrainedVal.set_float32_value(max_);
            } else if (!step_ || std::fmod(s - min_, step_) != 0) {
                constrainedVal.set_float32_value(s - std::fmod(s - min_, step_));
            }
            return constrainedVal;
        }
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     */
    void toProto(catena::Constraint& constraint) const override {

        if constexpr(std::is_same<T, int32_t>::value) {
            constraint.set_type(catena::Constraint::INT_RANGE);
            constraint.mutable_int32_range()->set_min_value(min_);
            constraint.mutable_int32_range()->set_max_value(max_);
            constraint.mutable_int32_range()->set_step(step_);
            constraint.mutable_int32_range()->set_display_min(display_min_);
            constraint.mutable_int32_range()->set_display_max(display_max_);
        } 

        if constexpr(std::is_same<T, float>::value) {
            constraint.set_type(catena::Constraint::FLOAT_RANGE);
            constraint.mutable_float_range()->set_min_value(min_);
            constraint.mutable_float_range()->set_max_value(max_);
            constraint.mutable_float_range()->set_step(step_);
            constraint.mutable_float_range()->set_display_min(display_min_);
            constraint.mutable_float_range()->set_display_max(display_max_);
        }
    }

    /**
     * @brief This constraint is a range constraint so return true
     * @return true
     */
    bool isRange() const override { return true; }

    /**
     * @brief check if the constraint is shared
     * @return true if the constraint is shared
     */
    bool isShared() const override { return shared_; }

    /**
     * @brief get the constraint oid
     * @return the oid of the constraint
     */
    const std::string& getOid() const override { return oid_; }

private:
    T min_;         ///< the minimum value
    T max_;         ///< the maximum value
    T step_;        ///< the step size
    T display_min_; ///< the minimum value to display
    T display_max_; ///< the maximum value to display
    bool strict_;   ///< should the value be constrained on apply
    bool shared_;   ///< is the constraint shared
    std::string oid_; ///< the oid of the constraint
};

} // namespace common
} // namespace catena