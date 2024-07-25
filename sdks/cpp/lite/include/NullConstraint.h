#pragma once

/**
 * @file NullConstraint.h
 * @brief A empty constraint to serve as a placeholder for params with no constraints
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-25
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <common/include/IConstraint.h>

/**
 * @brief Null constraint, does nothing
 */

class NullConstraint : public catena::common::IConstraint {
  public:
    NullConstraint() {}
    void toProto(google::protobuf::MessageLite& constraint) const override {};

    void apply(void* dst, void* src) const override {};
};
