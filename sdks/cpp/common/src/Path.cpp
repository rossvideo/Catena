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

//common
#include <Path.h>
#include <utils.h>
#include <Status.h>

#include <regex>

using catena::common::Path;
using Index = Path::Index;

Path::Path(const std::string &jptr) : segments_{} {
    // regex will split a well-formed json pointer into a sequence of Path Segments
    // typed according to the regex that matches the string for th segment
    // /- -- can be used as an array index
    // /any_string_that_uses_only_word_chars -- i.e. letters & underscores.
    // /string_with_~0_~1_escaped_chars -- string_with_~_/_escaped_chars
    // /291834719 -- just numbers
    //

    std::regex path_regex("((\\/-{1})|(\\/([\\w]|~[01])*))*");
    if (!std::regex_match(jptr, path_regex))  {
        std::stringstream why;
        why << __PRETTY_FUNCTION__ << "\n'" << jptr << "' is not a valid path";
        throw catena::exception_with_status(why.str(), catena::StatusCode::INVALID_ARGUMENT);
    }

    std::regex segment_regex("(\\/-{1})|(\\/([\\w]|~[01])*)");
    auto r_begin = std::sregex_iterator(jptr.begin(), jptr.end(), segment_regex);
    auto r_end = std::sregex_iterator();
    for (std::sregex_iterator it = r_begin; it != r_end; ++it) {
        std::smatch match = *it;
        if (it == r_begin && match.position(0) != 0) {
            std::stringstream why;
            why << __PRETTY_FUNCTION__ << "\n'" << jptr << " is invalid json pointer";
            throw catena::exception_with_status(why.str(), catena::StatusCode::INVALID_ARGUMENT);
        }

        // unescape and strip off leading solidus '/'
        std::string txt = unescape(match.str()).substr(1, std::string::npos);

        if (txt.compare("-") == 0) {
            // segment is an index with the one-past-the-end value
            Segment seg;
            seg.emplace<Index>(kEnd);
            segments_.push_back(seg);
        } else if (std::all_of(txt.cbegin(), txt.cend(), ::isdigit)) {
            // segment is an index
            Segment seg;
            seg.emplace<Index>(std::stoul(txt));
            segments_.push_back(seg);
        } else {
            // segment is a string
            Segment seg;
            seg.emplace<std::string>(txt);
            segments_.push_back(seg);
        }
    }
    front_ = cbegin();

    //     if (segments_.size() >= 1) {
    //     std::string seg = segments_[0];
    //     if (seg.compare("-") == 0) {
    //         // the one-past-the-end array index
    //         ans.emplace<Index>(kEnd);
    //     } else if (std::all_of(seg.begin(), seg.end(), ::isdigit)) {
    //         // segment is all digits, so convert to Index
    //         ans.emplace<Index>(std::stoul(seg));
    //     } else {
    //         // segment is a string
    //         ans.emplace<std::string>(seg);
    //     }
    // } else {
    //     // return empty string
    //     ans.emplace<std::string>("");
    // }
}

Path::Path(const char *literal) : Path(std::string(literal)) {}

bool Path::front_is_index() const {
    bool ans = false;
    if (front_ != cend() && std::holds_alternative<Index>(*front_)) {
        ans = true;
    }
    return ans;
}

bool Path::front_is_string() const {
    bool ans = false;
    if (front_ != cend() && std::holds_alternative<std::string>(*front_)) {
        ans = true;
    }
    return ans;
}

Index Path::front_as_index() const {
    Index ans = kError;
    if (front_is_index()) {
        ans = std::get<Index>(*front_);
    }
    return ans;
}

const std::string& Path::front_as_string() const {
    static const std::string Error{""};
    if (front_is_string()) {
        return std::get<std::string>(*front_);
    } else {
        return Error;
    }
}

void Path::pop() noexcept {
    if (front_ != cend()) {
        ++front_;
    }
}

Index Path::walked() const noexcept {
    return front_ - cbegin();
}

void Path::escape(std::string &str) {
    subs(str, "~", "~0");
    subs(str, "/", "~1");
}

std::string Path::unescape(const std::string &str) {
    std::string ans(str);
    subs(ans, "~0", "~");
    subs(ans, "~1", "/");
    return ans;
}

std::string Path::fqoid() {
    std::stringstream ans{""};
    for (auto it = segments_.cbegin(); it != segments_.cend(); ++it) {
        if (std::holds_alternative<Index>(*it)) {
            Index idx = std::get<Index>(*it);
            if (idx == kEnd) {
                ans << "/-";
            } else {
                ans << "/" << idx;
            }
        }
        if (std::holds_alternative<std::string>(*it)) {
            ans << "/" << std::get<std::string>(*it);
        }
    }
    return ans.str();
}

Path operator"" _path(const char *lit, std::size_t sz) {
    return Path(lit);
}
