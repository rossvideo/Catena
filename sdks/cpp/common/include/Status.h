#pragma once

/**
 * @brief Communicates failure status using gRPC's codes
 * @file Status.h
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

#include <grpcpp/support/status_code_enum.h>

#include <sstream>
#include <stdexcept>

namespace catena {
struct exception_with_status : public std::runtime_error {
  inline explicit exception_with_status(const std::string &why,
                                        grpc::StatusCode s)
      : std::runtime_error{why}, status{s} {}

  const grpc::StatusCode status;
};

} // namespace catena

/**
 * @brief creates a catena::exception_with_status object and throws it.
 * @todo condition whether __FILE__ and __LINE__ are reported on the build type
 * this info should be restricted to dev and dbg builds and not go into 
 * release / production ones.
 * 
 */
#define BAD_STATUS(msg, status)                                                \
  do {                                                                         \
    static_assert(status != grpc::StatusCode::OK,                              \
                  "Only use with error status codes");                         \
    std::stringstream why;                                                     \
    why << __FILE__ << ':' << __LINE__ << '\n' << msg;                         \
    throw catena::exception_with_status(why.str(), status);                    \
  } while (false)
