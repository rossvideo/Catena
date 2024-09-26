#pragma once

/**
 * @file TypeTraits.h
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @brief TypeTraits for providing compile time type information for structs and std::variant objects.
 * @version 0.1
 * @date 2024-01-23
 *
 * @copyright not asserted
 *
 */

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

#include <interface/device.pb.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <functional>
#include <unordered_map>

using std::size_t;

namespace catena {
namespace full {

struct FieldInfo;  // forward reference

/**
 * @brief holds information about data structures
 *
 */
struct StructInfo {
    std::string name;              /**< the data structure's name */
    std::vector<FieldInfo> fields; /**< name and offset info per field */
};

// class TypeTraits;  // forward reference

/**
 * @brief holds name and value information about a stucture's field.
 *
 */
class ParamAccessor;  // forward reference

struct FieldInfo {
    std::string name; /**< the field's name */
    int offset;       /**< the offset to the field's data from struct base */

    /**
     *  method returns type info of nested struct
     */
    std::function<StructInfo()> getStructInfo;

    /**
     * @brief recursive call into ParamAccessor::getValue<T> for nested structs
     */
    std::function<void(void* dstAddr, const ParamAccessor*)> wrapGetter;

    /**
     * @brief recursive call into ParamAccessor::setValue<T> for nested structs
     */
    std::function<void(ParamAccessor*, const void* srcAddr)> wrapSetter;

    /**
     * @brief class for field conversion to / back from protobuf.
     *
     * 'new' must beused to initialize the vtable.
     *
     */
    // std::shared_ptr<TypeTraits> fieldType;

    /**
     * @brief Sets the field's name.
     *
     * @param fieldName value to set it to.
     */
    void setName(const char* fieldName) {
        if (fieldName[0] == ' ')
            fieldName++;  // Result of define macro expansion, we fix it here.
        name = std::string(fieldName);
    }
};
struct VariantMemberInfo {
    size_t index;                              /**< index of the member in the variant */
    std::function<void*(void* dst)> set;       /**< function to set the variant */
    std::function<StructInfo()> getStructInfo; /**< type info of nested struct */
    std::function<void(void* dstAddr, const ParamAccessor*)> wrapGetter;
    std::function<void(ParamAccessor*, const void* srcAddr)> wrapSetter;
};

struct VariantInfo {
    std::string name;                                           /**< the variant's name */
    std::vector<std::string> lookup;                            /**< index to member type name */
    std::unordered_map<std::string, VariantMemberInfo> members; /**< name to member Info map */
};


/**
 * @brief determine at compile time if a type T has a getType method.
 *
 * default is false
 *
 * @todo refactor using C++ 20 concepts to make the code a bit clearer
 *
 * @tparam T
 * @tparam typename
 */
template <typename T, typename = void> constexpr bool has_getStructInfo{};

/**
 * @brief specialization for types that do have getStructInfo method
 *
 * @tparam T
 */
template <typename T>
constexpr bool has_getStructInfo<T, std::void_t<decltype(std::declval<T>().getStructInfo())>> = true;

/**
 * @brief returns the getStructInfo method for types that have it,
 * otherwise returns a function that returns an empty StructInfo object.
 */
template <typename T> std::function<catena::full::StructInfo()> getStructInfoFunction() {
    if constexpr (catena::full::has_getStructInfo<T>) {
        return T::getStructInfo;
    } else {
        return []() -> catena::full::StructInfo { return catena::full::StructInfo{}; };
    }
}

/**
 * @brief determine at compile time if a type T has a getStructInfo method.
 *
 * default is false
 *
 * @todo refactor using C++ 20 concepts to make the code a bit clearer
 *
 * @tparam T
 * @tparam typename
 */
template <typename T, typename = void> constexpr bool has_getVariant{};

/**
 * @brief specialization for types that do have getStructInfo method
 *
 * @tparam T
 */
template <typename T>
constexpr bool has_getVariant<T, std::void_t<decltype(std::declval<T>().getVariant())>> = true;

/**
 * @brief returns the getStructInfo method for types that have it,
 * otherwise returns a function that returns an empty StructInfo object.
 */
template <typename T> std::function<catena::full::StructInfo()> getVariantFunction() {
    if constexpr (catena::full::has_getVariant<T>) {
        return T::getStructInfo;
    } else {
        return []() -> catena::full::StructInfo { return catena::full::StructInfo{}; };
    }
}
}  // namespace full
}  // namespace catena
