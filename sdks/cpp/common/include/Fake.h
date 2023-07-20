#pragma once

/**
 * @brief Fake multi-threading classes
 * @file DeviceModel.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

namespace catena {

/**
 * @brief a do-nothing Mutex that only exists to get optimized out.
 *
 */
struct FakeMutex {};

/**
 * @brief a do-nothing lock guard that only exists to get optimized out.
 *
 */
template <typename MTX> class FakeLockGuard {
  public:
    explicit FakeLockGuard(MTX &mtx) {}
};
}  // namespace catena