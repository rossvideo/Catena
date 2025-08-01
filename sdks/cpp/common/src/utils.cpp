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

// common
#include <utils.h>

#include <cassert>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

std::string catena::readFile(std::filesystem::path path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);  // lock the file
    const auto sz = std::filesystem::file_size(path);           // get its size
    std::string ans(sz, '\0');                                  // create our buffer
    file.read(ans.data(), sz);                                  // read the file
    return ans;  // return the result - copy elision should prevent copy ctor
                 // being called
}

/**
 * @brief Substitutes all occurences of one char sequence in a string with
 * another.
 * 
 * Times out after 1000 iterations.
 *
 * @param str in/out the string to work on
 * @param seq in sequence to match
 * @param rep in sequence to replace the match
 */
void catena::subs(std::string& str, const std::string& seq, const std::string& rep) {
    if (seq.length() > 0) { // Sanitizing input
        std::ostringstream oss;
        std::size_t prev = 0, pos = 0;
        uint32_t timeout = 0; // Timeout counter to prevent infinite loop
        while (pos != std::string::npos && timeout++ < 1000) {
            prev = pos;
            pos = str.find(seq, pos);  // locate target sequence in input
            // Found
            if (pos != std::string::npos) {
                oss << str.substr(prev, pos - prev);  // write what came before
                oss << rep;                           // write the replacement
                pos += seq.size();                    // skip over the target sequence
            }
        }
        oss << str.substr(prev);  // write to the end of the input string
        str = oss.str();
    }
}

void catena::split(std::vector<std::string>& out, const std::string& str, const std::string& delim) {
    std::size_t prev = 0, pos = 0;
    out.clear();
    if (delim.length() > 0) {
        uint32_t timeout = 0; // Timeout counter to prevent infinite loop
        while (pos != std::string::npos && timeout++ < 1000) {
            prev = pos;
            pos = str.find(delim, pos);  // locate target sequence in input
            if (pos != std::string::npos) {
                out.emplace_back(str.substr(prev, pos - prev)); // write what came before
                pos += delim.size();                            // skip over the target sequence
            }
        }
    }
    out.emplace_back(str.substr(prev));  // write to the end of the input string
}


std::string catena::to_base64(const std::string& binary) {

    std::string encoded;
    int val = 0, valb = -6;
    for (uint8_t c : binary) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    return encoded;
}

std::string catena::from_base64(const std::string& encoded) {
    std::string decoded;
    int val = 0, valb = -8;
    for (unsigned char c : encoded) {
        if (c == '=') break;
        auto pos = strchr(base64_chars, c);
        if (pos == nullptr) continue; // Skip invalid characters
        val = (val << 6) + (pos - base64_chars);
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}