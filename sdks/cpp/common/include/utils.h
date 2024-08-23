#pragma once

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

/**
 * @brief Utility functions.
 * @file utils.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <filesystem>
#include <string>

namespace catena {

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
void subs(std::string &str, const std::string &seq, const std::string &rep);


}  // namespace catena
