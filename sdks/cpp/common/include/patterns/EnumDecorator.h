#pragma once


#include <ReflectionMacros.h>


#include <cstdint>
#include <unordered_map>
#include <string>
#include <type_traits>

namespace catena {
namespace patterns {

/**
 * @brief Provides an enum with useful extra functionality.
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
template <typename E> class EnumDecorator {
    // cause compile time error if E is not an enum
    static_assert(std::is_enum<E>::value, "EnumDecorator requires an enum type");

  public:
    using etype = E;                                      /*< template's enum parameter */
    using utype = typename std::underlying_type<E>::type; /*< underlying base type */
    using FwdMap = std::unordered_map<E, std::string>;    /*< forward map of enum values to strings */
    using RevMap = std::unordered_map<std::string, E>;    /*< reverse map of strings to enum values */

  private:
    E value_;                    /*< the current enum value */
    static const FwdMap fwdMap_; /*< forward map of enum values to strings */

  public:
    /**
     * @brief Default constructor.
     * Value of object will be 0.
     *
     */
    EnumDecorator() : value_{static_cast<E>(0)} {}

    /**
     * @brief Construct from an enum value.
     *
     * @param value the enum value
     */
    EnumDecorator(E value) : value_{value} {}


    /**
     * @brief Construct from a string.
     *
     * Beware, the default value is set to 0, which may not be a valid enum value.
     *
     * @param str the string representation of the enum value
     */
    EnumDecorator(const std::string& str) : value_{static_cast<E>(0)} {
        const RevMap& revMap = getReverseMap();
        auto it = revMap.find(str);
        if (it != revMap.end()) {
            value_ = it->second;
        }
    }

    /**
     * @brief provide the reverse map of strings to enum values
     *
     * lazy implementation
     *
     * @returns the reverse map
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
     * @brief construct from a value of the enum's underlying integral type.
     *
     * Sets the value of the object to 0 if the integral value is not valid.
     *
     * @param value the integral value
     */
    EnumDecorator(utype value) : value_{static_cast<E>(value)} {
        auto it = fwdMap_.find(value_);
        if (it == fwdMap_.end()) {
            value_ = static_cast<E>(0);
        }
    }


    /**
     * @brief EnumDecorators have copy semantics.
     *
     */
    EnumDecorator(const EnumDecorator& other) = default;

    /**
     * @brief EnumDecorators have copy semantics.
     *
     */
    EnumDecorator& operator=(const EnumDecorator& other) = default;

    /**
     * @brief EnumDecorators have move semantics.
     *
     */
    EnumDecorator(EnumDecorator&& other) = default;

    /**
     * @brief EnumDecorators have move semantics.
     *
     */
    EnumDecorator& operator=(EnumDecorator&& other) = default;

    /**
     * @brief value accessor.
     *
     * @returns the current enum value.
     *
     */
    E value() const { return value_; }

    /**
     * @brief value accessor, alternative syntax.
     * @returns the current enum value.
     */
    E operator()() const { return value_; }


    /**
     * @brief cast to the enum's underlying integral type
     *
     * @returns the current enum value or the default value if the object is not valid.
     */
    operator utype() const { return static_cast<utype>(value()); }

    /**
     * @brief cast to a string
     */
    operator std::string() const {
        auto it = fwdMap_.find(value_);
        if (it != fwdMap_.end()) {
            return it->second;
        } else {
            return std::string();
        }
    }

    /**
     * @brief get the string representation of the enum value
     */
    const std::string& toString() const {
        auto it = fwdMap_.find(value_);
        if (it != fwdMap_.end()) {
            return it->second;
        } else {
            static std::string empty;
            return empty;
        }
    }

    /**
     * @brief equality operator
     *
     * @returns true if the enum values are equal, false if they are different, and false if either object is
     * not valid.
     */
    bool operator==(const EnumDecorator& other) const { return value() == other.value(); }

    /**
     * @brief inequality operator
     *
     * @returns true if the enum values are different, false if they are equal, and false if either object is
     * not valid.
     */
    bool operator!=(const EnumDecorator& other) const { return value_ != other.value_; }
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
