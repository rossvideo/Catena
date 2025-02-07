#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
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

/** @example path.cpp demonstrates use of a variety of json-pointers that are useful with catena models. */
#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace catena {
namespace common {

using SegmentType = std::variant<std::string, size_t, int>;

/**
 * @brief Converts json pointers to items within the data model to
 * a path of "segments" that can be iterated over.
 * 
 * Not all json pointers are supported, specifically the empty string has
 * no meaning within a Catena use case.
 *
 */
class Path {
  public:
    /**
     * @brief Type for indices, unsigned int.
     */
    using Index = size_t;

    /**
     * @brief Segment can be interpreted either as an oid (string),
     * or array index (std::size_t). The "one past the end" index
     * is flagged by the value kEnd.
     *
     */
    using Segment = typename std::variant<Index, std::string>;

    /**
     * @brief used to signal one-past-the-end array size
     */
    static constexpr Index kEnd = Index(-1);

    /**
     * @brief used to signal an error
     */
    static constexpr Index kError = kEnd - 1;

  public:

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
    explicit Path(const std::string& jptr);

    /**
     * @brief Construct a new Path object
     *
     * @param literal an escaped json-pointer,
     * i.e. '/' replaced by "~1" and '~' by "~0"
     */
    explicit Path(const char* literal);

    /**
     * @brief Construct a Path from initializer list
     * @param args List of arguments that are either Index or string types
     */
    explicit Path(std::initializer_list<SegmentType> args) : segments_{}, frontIdx_{0} {
        for (const auto& arg : args) {
            push_back(arg);
        }
    }

    /**
     * @brief return the number of segments in the Path
     *
     * @return number of segments
     */
    inline Index size() const { return segments_.size() - frontIdx_; }

    /**
     * @brief return true if the Path is empty
     *
     * @return true if the Path is empty
     */
    inline bool empty() const { return frontIdx_ >= segments_.size(); }

    /**
     * @return true if the front of the path is a string, false if it's an Index or empty.
     */
    bool front_is_string() const;

    /**
     * @return true if the front of the path is an Index, false if it's a string or empty.
     */
    bool front_is_index() const;

    /**
     * @return front of Path as a string.
     * @throws catena::exception_with_status if path is empty or front is not a string.
     * @code
     * // recommended usage
     * std::string oid = p.front_is_string() ? p.front_as_string() : "";
     * if (oid == "") { // error handling here }
     * @endcode
     */
    const std::string& front_as_string() const;

    /**
     * @return front of path as an Index.
     * @throws catena::exception_with_status if path is empty or front is not an Index.
     * @code
     * // recommended usage
     * Path::Index idx = p.front_is_index() ? p.front_as_index() : Path::kError;
     * if (idx == Path::kError) { // error handling here }
     */
    Index front_as_index() const;

    /**
     * @brief return a string representation of the Path
     * @param leading_slash if true, include leading '/' in output (defaults to true)
     * @return std::string the path as a string
     * 
     * any popped segments are not included in the returned string.
     */
    std::string toString(bool leading_slash = false) const;

    /**
     * @brief return a fully qualified, albeit escaped oid
     * @return std::string
     * 
     * The fully qualified oid is rooted at the device and includes all popped segments.
     */
    std::string fqoid() const;

    /**
     * @brief pop the front of the path.
     * Decreases the length of the path by 1 unless the path is already empty in which case it does nothing.
     */
    void pop() noexcept;

    /**
     * @brief show how much of the path has been consumed
     * @return number of segments that have been popped
     */
    Index walked() const noexcept;

    /**
     * @brief restore the Path to start at its original front
     */
    inline void rewind() noexcept {frontIdx_ = 0;}

    /**
     * @brief restore the Path to it's state before the last pop
     */
    inline void unpop() noexcept {if (frontIdx_ > 0) {--frontIdx_;}}

    /**
     * @brief Add a new segment to the path
     * @param segment Either an Index or string to add as a new segment
     */
    void push_back(SegmentType segment) {
        std::visit(
            [this](const auto& val) {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                    if (val == "-") {
                        segments_.emplace_back(std::in_place_type<Index>, kEnd);
                    } else {
                        std::string str = val;
                        escape(str);
                        segments_.emplace_back(std::in_place_type<std::string>, str);
                    }
                } else {
                    segments_.emplace_back(std::in_place_type<Index>, static_cast<Index>(val));
                }
            },
            segment
        );
    }


  private:
    using Segments = std::vector<Segment>;
    Segments segments_{}; /**< the path split into its components */
    std::size_t frontIdx_{0}; /**< index of current front of path */

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

}  // namespace common
}  // namespace catena

catena::common::Path operator"" _path(const char* lit, std::size_t sz);
