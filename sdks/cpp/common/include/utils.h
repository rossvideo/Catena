#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

/**
 * @brief Utility functions.
 * @file utils.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <filesystem>
#include <string>
#include <vector>

namespace catena {

constexpr static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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
 * Times out after 1000 iterations.
 *
 * @param str in/out the string to work on, done in place
 * @param seq in sequence to match
 * @param rep in sequence to replace the match
 */
void subs(std::string &str, const std::string &seq, const std::string &rep);

/**
 * @brief Splits a string int a vector of strings based on a delimiter.
 * 
 * Times out after 1000 iterations.
 * 
 * @param out   io The vector to store the new split string in.
 * @param str   in The string to split.
 * @param delim in The character to split the string at.
 */
void split(std::vector<std::string>& out, const std::string& str, const std::string& delim);

/**
 * @brief Converts a binary string to a base64 encoded string.
 * 
 * @param binary The binary string to encode.
 * @return The base64 encoded string.
 */
std::string to_base64(const std::string& binary);

/**
 * @brief Converts a base64 encoded string to a binary string.
 * 
 * @param encoded The base64 encoded string to decode.
 * @return The decoded binary string.
 */
std::string from_base64(const std::string& encoded);

}  // namespace catena
