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

        std::string txt = unescape(match.str());
        // strip off the leading solidus '/'
        segments_.push_back(txt.substr(1, std::string::npos));
    }
    front_ = cbegin();
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
