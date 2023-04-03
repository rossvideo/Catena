#pragma once

/**
 * @brief Utility functions.
 * @file utils.h
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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace catena {


/**
 * @brief prints a protobuf message as JSON
 *
 * @param msg the protobuf message
 * @return std::string JSON representation of m
 */
std::string printJSON(const google::protobuf::Message& msg);

/**
 * @brief parses protobuf message to JSON
 *
 * @param msg JSON message to parse
 * @param m output protobuf message
 */
void parseJSON(const std::string& msg, google::protobuf::Message& m);

/**
 * @brief reads a file into a std::string
 *
 * @param path path/to/the/file
 * @returns string containing contents of file
 *
 * N.B relies on C++11 copy elision to be efficient!
 */
std::string readFile(std::filesystem::path path);

/**
 * @brief Substitutes all occurences of one char sequence in a string with
 * another.
 *
 * @param str in/out the string to work on, done in place
 * @param seq in sequence to match
 * @param rep in sequence to replace the match
 */
void subs(std::string& str, const std::string& seq, const std::string& rep);

}  // namespace catena
