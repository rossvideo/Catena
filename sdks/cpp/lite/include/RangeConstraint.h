#pragma once

/**
 * @file RangeConstraint.h
 * @brief A constraint that checks if a value is within a range
 * @author john.naylor@rossvideo.com, isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>
#include  <google/protobuf/message_lite.h>

using catena::common::IConstraint;

/**
 * @brief Range constraint, ensures a value is within a range.
 */
template <typename T> 
class RangeConstraint : public IConstraint {
public:
    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     */
    RangeConstraint(T min, T max, std::string oid, bool shared)
        : min_(min), max_(max), step_{}, display_min_{min}, display_max_{max} {
            setOid(oid);
            setShared(shared);
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

    void apply(void* dst, void* src) const override {
        auto& update = *reinterpret_cast<catena::Value*>(src);

        if constexpr(std::is_same<T, int32_t>::value) {
            std::cout << "applying int32_t constraint to: " << update.int32_value() << std::endl;

            // ignore the request if src is not valid
            if (!update.has_int32_value()) { return; }

            if (update.int32_value() < min_) {
                update.set_int32_value(min_);
            } else if (update.int32_value() > max_) {
                update.set_int32_value(max_);
                std::cout << "setting to max: " << update.int32_value() << std::endl;
            }
            reinterpret_cast<catena::Value*>(dst)->set_int32_value(update.int32_value());
        }
        
        if constexpr(std::is_same<T, float>::value) {
            // ignore the request if src is not valid
            if (!update.has_float32_value()) { return; }

            if (update.float32_value() < min_) {
                update.set_float32_value(min_);
            } else if (update.float32_value() > max_) {
                update.set_float32_value(max_);
            }
            reinterpret_cast<catena::Value*>(dst)->set_float32_value(update.float32_value());
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
