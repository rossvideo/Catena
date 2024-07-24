#pragma once

/**
 * @file RangeConstraint.h
 * @brief A constraint that checks if a value is within a range
 * @author john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>

/**
 * @brief Range constraint, ensures a value is within a range.
 */

template <typename T> class RangeConstraint : public IConstraint {
  public:
    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param value the value to constrain
     */
    RangeConstraint(T min, T max, T& value)
        : min_(min), max_(max), value_(value), step{}, display_min_{min}, display_max_{max} {}

    /**
     * @brief Construct a new Range Constraint object
     * @param min the minimum value
     * @param max the maximum value
     * @param value the value to constrain
     * @param step the step size
     * @param display_min the minimum value to display
     * @param display_max the maximum value to display
     */
    RangeConstraint(T min, T max, T& value, T step, T display_min, T display_max)
        : min_(min), max_(max), value_(value), step_(step), display_min_{display_min},
          display_max_{display_max} {}

    /**
     * @brief test if the value is within the range
     * @return true if the value is within the range, false otherwise
     */
    bool isValid() const override { return value_ >= min_ && value_ <= max_; }

    /**
     * @brief constrain the value to lie within the range
     */
    void constrain() override {
        if (value_ < min_) {
            value_ = min_;
        } else if (value_ > max_) {
            value_ = max_;
        }
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     * @note this is a template specialization for floating point types
     */
    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, void>::type
    void toProto(catena::MessageLite& msg) const override {
        auto& constraint = dynamic_cast<catena::Constraint&>(msg);
        constraint.set_type(catena::Constraint::FLOAT_RANGE);
        toProtoInner_(constraint);
    }

    /**
     * @brief serialize the constraint to a protobuf message
     * @param msg the protobuf message to populate
     * @note this is a template specialization for integer types
     */
    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, void>::type
    void toProto(catena::MessageLite& msg) const override {
        auto& constraint = dynamic_cast<catena::Constraint&>(msg);
        constraint.set_type(catena::Constraint::INT_RANGE);
        toProtoInner_(constraint);
    }

  private:
    T min_;         ///< the minimum value
    T max_;         ///< the maximum value
    T step_;        ///< the step size
    T display_min_; ///< the minimum value to display
    T display_max_; ///< the maximum value to display
    T& value_;      ///< the value to constrain

    /**
     * @brief serialize the constraint to a protobuf message
     * @param constraint the protobuf message to populate
     */
    void toProtoInner_(catena::Constraint& constraint) {
        constraint.mutable_range()->set_min(min_);
        constraint.mutable_range()->set_max(max_);
        constraint.mutable_range()->set_step(step_);
        constraint.mutable_range()->set_display_min(display_min_);
        constraint.mutable_range()->set_display_max(display_max_);
    }
};
