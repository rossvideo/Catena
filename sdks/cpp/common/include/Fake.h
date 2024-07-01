#pragma once

#include <mutex>

namespace catena {
namespace common {
/**
 * a fake lock for use in recursive function calls
 */
struct FakeLock {
    FakeLock(std::mutex &) {}
};
}  // namespace common
}  // namespace catena 