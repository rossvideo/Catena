#pragma once

/**
 * @brief Handles Path objects used to uniquely identify and access OIDs
 * @file Path.h
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


 #include <iostream>
 #include <string>
 #include <deque>
 #include <variant>

namespace catena {
/**
 * @brief Handles Path objects used to uniquely identify and access OIDs
 * 
 */
class Path {
public:
    /**
     * @brief what we split the path into
     * 
     */
    using Segments = std::deque<std::string>;

    /**
     * @brief type of index path segments
     * 
     */
    using Index = Segments::size_type;

    /**
     * @brief used to signal one-past-the-end array size
     *
     */
    static constexpr Index kEnd = Index(-1);

    /**
     * @brief Segments can be interpreted either as oids (strings), 
     * or array indices (std::size_t). The "one past the end" index
     * is flagged by the value kEnd.
     * 
     */
    using Segment = typename std::variant<Index, std::string>;


    Path() = default;
    Path(const Path& other) = default;
    Path& operator=(const Path& rhs) = default;
    Path(Path&& other) = default;
    Path& operator=(Path&& rhs) = default;

    /**
     * @brief Construct a new Path object.
     * 
     * 
     * @param path an escaped json-pointer, 
     * i.e. '/' replaced by "~1" and '~' by "~0"
     */
    explicit Path(const std::string& path);

    /**
     * @brief Construct a new Path object
     * 
     * @param literal an escaped json-pointer, 
     * i.e. '/' replaced by "~1" and '~' by "~0"
     */
    explicit Path(const char* literal);

    /**
     * @brief iterator to the start of our segmented Path.
     *
     * @return std::vector::iterator
     */
    inline Segments::iterator begin() { return segments_.begin(); }

    /**
     * @brief iterator to the end of our segmented Path.
     *
     * @return std::vector::iterator
     */
    inline Segments::iterator end() { return segments_.end(); }

    /**
     * @brief return the number of segments in the Path
     *
     * @return number of segments
     */
    inline Index size() { return segments_.size(); }

    /**
     * @brief take the front off the path and return it.
     * @return unescaped component at front of the path, 
     * design intent the returned value can be used as an oid lookup,
     * or an array index.
     * Will be empty string if nothing to pop, or the original path
     * was "/", or "".
     */
    Segment pop_front() noexcept;

private:
    Segments segments_; /**< the path split into its components */
    
    /**
     * @brief replace / and ~ characters with ~1 & ~0
     * 
     * @param str in|out
     */
    void escape (std::string& str);

    /**
     * @brief replace ~0 and ~1 with ~ and /
     * 
     * @param str 
     * @return unescaped version of str
     */
    std::string unescape (const std::string& str);
};
} // namespace catena

std::unique_ptr<catena::Path> operator"" _path(const char* lit, std::size_t sz);
