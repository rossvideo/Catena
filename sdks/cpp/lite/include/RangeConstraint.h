#pragma once

/**
 * @file RangeConstraint.h
 * @brief A constraint that checks if a value is within a range
 * @author john.naylor@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>
#include  <google/protobuf/message_lite.h>


/**
 * @brief Range constraint, ensures a value is within a range
 * @tparam T int or float
 */
template <typename T> 
class RangeConstraint : public catena::common::IConstraint {
public:
    /**
     * @brief local alias for IConstraint
     */
    using IConstraint = catena::common::IConstraint;

public:
    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     */
    RangeConstraint(T min, T max, std::string oid, bool shared)
        : min_(min), max_(max), step_{1}, display_min_{min}, display_max_{max} {
        this->setOid(oid);
        this->setShared(shared);
    }

    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param step the step size
     * @param display_min the minimum value to display
     * @param display_max the maximum value to display
     */
    RangeConstraint(T min, T max, T step, T display_min, T display_max, std::string oid, bool shared)
        : min_(min), max_(max), step_(step), display_min_{display_min}, display_max_{display_max} {
            setOid(oid);
            setShared(shared);
        }

    /**
     * @brief applies constraint to src and writes constrained value to dst
     * @param src a catena::Value containing the value to apply the constraint to
     * @param dst a catena::Value to write the constrained value to
     */
    // TODO update apply
    void apply(void* dst, const void* src) const override {
        if constexpr(std::is_same<T, int32_t>::value) {
            auto& src_val = *reinterpret_cast<const catena::Value*>(src);
            // ignore the request if src is not valid
            if (!src_val.has_int32_value()) { return; }
            
            auto& update = *reinterpret_cast<T*>(dst);
            // constrain if not within allowed range
            if (src_val.int32_value() < min_) {
                update = min_;
            } else if (src_val.int32_value() > max_) {
                update = max_;
            } else {
                update = src_val.int32_value();
            }
        }
        
        if constexpr(std::is_same<T, float>::value) {
            auto& src_val = *reinterpret_cast<const catena::Value*>(src);
            // ignore the request if src is not valid
            if (!src_val.has_float32_value()) { return; }

            auto& update = *reinterpret_cast<T*>(dst);
            // constrain if not within allowed range
            if (src_val.float32_value() < min_) {
                update = min_;
            } else if (src_val.float32_value() > max_) {
                update = max_;
            } else {
                update = src_val.float32_value();
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
