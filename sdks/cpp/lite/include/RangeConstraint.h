#pragma once

/**
 * @file RangeConstraint.h
 * @brief A constraint that checks if a value is within a range
 * @author john.naylor@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <lite/include/IConstraint.h>

/**
 * @brief A constraint that checks if a value is within a range
 */

template <typename T> class RangeConstraint : public IConstraint {
  public:
    RangeConstraint(T min, T max, T& value)
        : min_(min), max_(max), value_(value), step{}, display_min_{min}, display_max_{max} {}
    RangeConstraint(T min, T max, T& value, T step, T display_min, T display_max)
        : min_(min), max_(max), value_(value), step_(step), display_min_{display_min},
          display_max_{display_max} {}

    bool isValid() override { return value_ >= min_ && value_ <= max_; }

    void constrain() override {
        if (value_ < min_) {
            value_ = min_;
        } else if (value_ > max_) {
            value_ = max_;
        }
    }

    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, void>::type
    void toProto(catena::Constraint& constraint) const override {
        constraint.set_type(catena::Constraint::FLOAT_RANGE);
        toProtoInner_(constraint);
    }

    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, void>::type
    void toProto(catena::Constraint& constraint) const override {
        constraint.set_type(catena::Constraint::INT_RANGE);
        toProtoInner_(constraint);
    }

  private:
    T min_;
    T max_;
    T step_;
    T display_min_;
    T display_max_;
    T& value_;

    void toProtoInner_(catena::Constraint& constraint) {
        constraint.mutable_range()->set_min(min_);
        constraint.mutable_range()->set_max(max_);
        constraint.mutable_range()->set_step(step_);
        constraint.mutable_range()->set_display_min(display_min_);
        constraint.mutable_range()->set_display_max(display_max_);
    }
};
