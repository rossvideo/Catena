#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

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

//lite
#include <Device.h>

#include <google/protobuf/message_lite.h>

namespace catena {
namespace lite {

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
    RangeConstraint(T min, T max, std::string oid, bool shared, Device& dm)
        : IConstraint{oid, shared}, min_(min), max_(max), step_{1}, 
        display_min_{min}, display_max_{max} {
        dm.addItem<common::ConstraintTag>(oid, this);
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
    RangeConstraint(T min, T max, std::string oid, bool shared, catena::common::IParam* parent)
        : IConstraint{oid, shared}, min_(min), max_(max), step_{1}, 
        display_min_{min}, display_max_{max} {
        parent->addConstraint(oid, this);
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
     * @param dm the device to add the constraint to
     */
    RangeConstraint(T min, T max, T step, T display_min, T display_max, std::string oid,
        bool shared, Device& dm)
        : IConstraint{oid, shared}, min_(min), max_(max), step_(step),
        display_min_{display_min}, display_max_{display_max} {
        dm.addItem<common::ConstraintTag>(oid, this);
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
        bool shared, catena::common::IParam* parent)
        : IConstraint{oid, shared}, min_(min), max_(max), step_(step),
        display_min_{display_min}, display_max_{display_max} {
        parent->addConstraint(oid, this);
    }

    /**
     * @brief default destructor
     */
    virtual ~RangeConstraint() = default;

    /**
     * @brief applies range constraint to a catena::Value
     * @param src a catena::Value to apply the constraint to
     */
    void apply(void* src) const override {
        auto& src_val = *reinterpret_cast<catena::Value*>(src);

        if constexpr(std::is_same<T, int32_t>::value) {
            // ignore the request if src is not valid
            if (!src_val.has_int32_value()) { return; }
            
            // constrain if not within allowed range
            if (src_val.int32_value() < min_) {
                src_val.set_int32_value(min_);
            } else if (src_val.int32_value() > max_) {
                src_val.set_int32_value(max_);
            }
        }

        if constexpr(std::is_same<T, float>::value) {
            // ignore the request if src is not valid
            if (!src_val.has_float32_value()) { return; }

            // constrain if not within allowed range
            if (src_val.float32_value() < min_) {
                src_val.set_float32_value(min_);
            } else if (src_val.float32_value() > max_) {
                src_val.set_float32_value(max_);
            }
        }
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     */
    void toProto(google::protobuf::MessageLite& msg) const override {
        auto& constraint = dynamic_cast<catena::Constraint&>(msg);

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

private:
    T min_;         ///< the minimum value
    T max_;         ///< the maximum value
    T step_;        ///< the step size
    T display_min_; ///< the minimum value to display
    T display_max_; ///< the maximum value to display
};

} // namespace lite
} // namespace catena