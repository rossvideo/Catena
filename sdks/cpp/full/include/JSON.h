#pragma once

/**
 * @brief Convenience functions for working with JSON and files.
 * @file JSON.h
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

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include <string>

namespace catena {
namespace full {

/**
 * @brief prints a protobuf message as JSON
 *
 * @param msg the protobuf message
 * @return std::string JSON representation of m
 */
std::string printJSON(const google::protobuf::Message &msg);

/**
 * @brief parses protobuf message to JSON
 *
 * @param msg JSON message to parse
 * @param m output protobuf message
 */
void parseJSON(const std::string &msg, google::protobuf::Message &m);

} // namespace full
}  // namespace catena
