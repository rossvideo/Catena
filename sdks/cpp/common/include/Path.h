#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

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
 * @brief Handles Path objects used to uniquely identify and access OIDs
 * @file Path.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <iostream>
#include <string>
#include <deque>
#include <variant>
#include <memory>

namespace catena {
namespace common {
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
     * @throw catena::exception_with_status INVALID_ARGUMENT if path is not a valid json-pointer
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
    inline Index size() const { return segments_.size(); }

    /**
     * @brief take the front off the path and return it.
     * @return unescaped component at front of the path,
     * design intent the returned value can be used as an oid lookup,
     * or an array index.
     * Will be empty string if nothing to pop, or the original path
     * was "/", or "".
     */
    Segment pop_front() noexcept;

    /**
     * @brief return the front of the path.
     * @return unescaped component at front of the path,
     * design intent the returned value can be used as an oid lookup,
     * or an array index.
     * Will be empty string if path is "/", or "".
     */
    Segment front() noexcept;

    /**
     * @brief escapes the oid then adds it to the end of the path.
     *
     * @param oid
     */
    void push_back(const std::string& oid);

    /**
     * @brief return a fully qualified, albeit escaped oid
     *
     * @return std::string
     */
    std::string fqoid();

  private:
    Segments segments_; /**< the path split into its components */

    /**
     * @brief replace / and ~ characters with ~1 & ~0
     *
     * @param str in|out
     */
    void escape(std::string& str);

    /**
     * @brief replace ~0 and ~1 with ~ and /
     *
     * @param str
     * @return unescaped version of str
     */
    std::string unescape(const std::string& str);
};

}  // namespace full
}  // namespace catena

catena::common::Path operator"" _path(const char* lit, std::size_t sz);
