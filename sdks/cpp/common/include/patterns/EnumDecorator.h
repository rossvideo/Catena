#pragma once

/*
 * Copyright 2024 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Provides an enum with useful extra functionality.
 * @file EnumDecorator.h
 *
 * - construct from string
 * - construct from integral type
 * - convert to integral type
 * - convert to string
 * - equality operators
 * - map of enum values to strings
 *
 * There are some restrictions on the enum type:
 *
 * - no duplicate enum values
 * - no duplicate description strings
 * - no user-assignable values, the enum values must be sequential starting from 0
 * - users are strongly advised to use the ENUMDECORATOR_DECLARATION and ENUMDECORATOR_DEFINITION macros
 *
 * @author john.naylor@rossvideo.com
 * @copyright © 2024 Ross Video Ltd.
 */

 /**
  * @example enum_decorator.cpp
  * Provides example of using EnumDecorator.
  */

// common
#include <ReflectionMacros.h>

// std
#include <cstdint>
#include <unordered_map>
#include <string>
#include <type_traits>

namespace catena {

/**
 * @brief A collection of classes which implement design patterns for function
 * object management.
 */
namespace patterns {

/**
 * @brief Enhances default C++ enums by adding functionality for converting
 * between enum values, strings and integral types.
 */
template <typename E> class EnumDecorator {
    // cause compile time error if E is not an enum
    static_assert(std::is_enum<E>::value, "EnumDecorator requires an enum type");

  public:
    using etype = E;                                      /*< template's enum parameter */
    using utype = typename std::underlying_type<E>::type; /*< underlying base type */
    using FwdMap = std::unordered_map<E, std::string>;    /*< forward map of enum values to strings */
    using RevMap = std::unordered_map<std::string, E>;    /*< reverse map of strings to enum values */

    /**
     * @brief Default constructor.
     * Value of object will be 0.
     */
    EnumDecorator() : value_{static_cast<E>(0)} {}

    /**
     * @brief Construct from an enum value.
     *
     * @param value The enum value.
     */
    EnumDecorator(E value) : value_{value} {}


    /**
     * @brief Construct from a string.
     *
     * Beware, the default value is set to 0, which may not be a valid enum
     * value.
     *
     * @param str The string representation of the enum value.
     */
    EnumDecorator(const std::string& str) : value_{static_cast<E>(0)} {
        const RevMap& revMap = getReverseMap();
        if (revMap.contains(str)) {
            value_ = revMap.at(str);
        }
    }

    /**
     * @brief provide the reverse map of strings to enum values.
     *
     * Lazy implementation.
     *
     * @returns The reverse map.
     */
    const RevMap& getReverseMap() {
        static RevMap revMap;
        if (revMap.empty()) {
            for (const auto& pair : fwdMap_) {
                revMap[pair.second] = pair.first;
            }
        }
        return revMap;
    }

    /**
     * @brief provide the forward map of enum values to strings
     *
     * @returns the forward map
     */
    const FwdMap& getForwardMap() const { return fwdMap_; }

    /**
     * @brief Construct from a value of the enum's underlying integral type.
     *
     * Sets the value of the object to 0 if the integral value is not valid.
     *
     * @param value The integral value.
     */
    EnumDecorator(utype value) : value_{static_cast<E>(value)} {
        if (!fwdMap_.contains(value_)) {
            value_ = static_cast<E>(0);
        }
    }


    /**
     * @brief EnumDecorators have copy semantics.
     */
    EnumDecorator(const EnumDecorator& other) = default;
    EnumDecorator& operator=(const EnumDecorator& other) = default;

    /**
     * @brief EnumDecorators have move semantics.
     */
    EnumDecorator(EnumDecorator&& other) = default;
    EnumDecorator& operator=(EnumDecorator&& other) = default;

    /**
     * @brief Value accessor.
     *
     * @returns The current enum value.
     */
    E value() const { return value_; }

    /**
     * @brief Value accessor, alternative syntax.
     * @returns The current enum value.
     */
    E operator()() const { return value_; }


    /**
     * @brief Cast to the enum's underlying integral type
     *
     * @returns The current enum value or the default value if the object is
     * not valid.
     */
    operator utype() const { return static_cast<utype>(value()); }

    /**
     * @brief Cast to a string.
     */
    operator std::string() const {
        if (fwdMap_.contains(value_)) {
            return fwdMap_.at(value_);
        } else {
            return std::string();
        }
    }

    /**
     * @brief Get the string representation of the enum value.
     */
    const std::string& toString() const {
        if (fwdMap_.contains(value_)) {
            return fwdMap_.at(value_);
        } else {
            static std::string empty;
            return empty;
        }
    }

    /**
     * @brief Equality operator.
     *
     * @returns True if the enum values are equal, False if they are different
     * or either object is not valid.
     */
    bool operator==(const EnumDecorator& other) const { return value() == other.value(); }

    /**
     * @brief inequality operator
     *
     * @returns true if the enum values are different, false if they are equal
     * or either object is not valid.
     */
    bool operator!=(const EnumDecorator& other) const { return value_ != other.value_; }

  private:
    /**
     * @brief The current enum value.
     */
    E value_;
    /**
     * @brief Forward map of enum values to strings.
     */
    static const FwdMap fwdMap_;
};

}  // namespace patterns
}  // namespace catena

/**
 * @def MAPPAIR formats a pair of enum value and string for the forward map
 */
#define MAPPAIR(x)                                                                                           \
    { enumName::ARGTYPE(x), ARGNAME(x) }

/**
 * @def ENUMDECORATOR declares an EnumDecorator for an enum type
 */
#define ENUMDECORATOR_DECLARATION(className, utype, ...)                                                     \
    enum class className##_e : utype{DOFOREACH(ARGTYPE, __VA_ARGS__)};
/**
 * @def ENUMDECORATOR defines the forward map for an EnumDecorator
 * N.B. because it defines a type alias, enumName, it can only be used once per translation unit
 * If you want to define multiple EnumDecorators in the same translation unit, you
 * must instantiate them by hand.
 */
#define ENUMDECORATOR_DEFINITION(className, utype, ...)                                                      \
    using enumName = className##_e;                                                                          \
    template <>                                                                                              \
    const typename catena::patterns::EnumDecorator<className##_e>::FwdMap                                    \
      catena::patterns::EnumDecorator<className##_e>::fwdMap_ = {DOFOREACH(MAPPAIR, __VA_ARGS__)};

#define INSTANTIATE_ENUM(m, x) m x
