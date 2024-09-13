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

Path::Path(const std::string &path) : segments_{} {
    // regex will split a well-formed json pointer into a sequence of strings
    // that all start with the solidus '/' character and contain these matches:
    // /- -- can be used as an array index
    // /any_string_that_uses_only_word_chars -- i.e. letters & underscores.
    // /string_with_~0_~1_escaped_chars -- string_with_~_/_escaped_chars
    // /291834719 -- just numbers
    //

    if (path.empty()) {
        std::stringstream why;
        why << __PRETTY_FUNCTION__ << "\npath is empty";
        throw catena::exception_with_status(why.str(), catena::StatusCode::INVALID_ARGUMENT);
    }

    std::regex path_regex("((\\/-{1})|(\\/([\\w]|~[01])*))*");
    if (!std::regex_match(path, path_regex))  {
        std::stringstream why;
        why << __PRETTY_FUNCTION__ << "\n'" << path << "' is not a valid path";
        throw catena::exception_with_status(why.str(), catena::StatusCode::INVALID_ARGUMENT);
    }

    std::regex segment_regex("(\\/-{1})|(\\/([\\w]|~[01])*)");
    auto r_begin = std::sregex_iterator(path.begin(), path.end(), segment_regex);
    auto r_end = std::sregex_iterator();
    for (std::sregex_iterator it = r_begin; it != r_end; ++it) {
        std::smatch match = *it;
        if (it == r_begin && match.position(0) != 0) {
            std::stringstream why;
            why << __PRETTY_FUNCTION__ << "\n'" << path << " is invalid json pointer";
            throw catena::exception_with_status(why.str(), catena::StatusCode::INVALID_ARGUMENT);
        }

        std::string txt = unescape(match.str());
        // strip off the leading solidus '/'
        segments_.push_back(txt.substr(1, std::string::npos));
    }
}

Path::Path(const char *literal) : Path(std::string(literal)) {}

void Path::pop_front() noexcept {
    segments_.pop_front();
}

Path::Segment Path::front() noexcept {
    Segment ans;
    if (segments_.size() >= 1) {
        std::string seg = segments_[0];
        if (seg.compare("-") == 0) {
            // the one-past-the-end array index
            ans.emplace<Index>(kEnd);
        } else if (std::all_of(seg.begin(), seg.end(), ::isdigit)) {
            // segment is all digits, so convert to Index
            ans.emplace<Index>(std::stoul(seg));
        } else {
            // segment is a string
            ans.emplace<std::string>(seg);
        }
    } else {
        // return empty string
        ans.emplace<std::string>("");
    }
    return ans;
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

void Path::push_back(const std::string &oid) {
    std::string back{oid};
    escape(back);
    segments_.push_back(back);
}

std::string Path::fqoid() {
    std::stringstream ans{""};
    for (auto it = segments_.begin(); it != segments_.end(); ++it) {
        ans << '/' << *it;
    }
    return ans.str();
}

Path operator"" _path(const char *lit, std::size_t sz) {
    return Path(lit);
}
