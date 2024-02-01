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

#include <device.pb.h>

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
  std::function<StructInfo()> getTypeInfo; /**< type info of nested struct */
  std::function<void(void* dstAddr, ParamAccessor*)> wrapGetter;
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
  size_t index; /**< index of the member in the variant */
  std::function<void*(void* dst)> set; /**< function to set the variant */
  std::function<StructInfo()> getTypeInfo; /**< type info of nested struct */
};

struct VariantInfo {
  std::string name; /**< the variant's name */
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
template <typename T, typename = void>
constexpr bool has_getStructInfo{};

/**
 * @brief specialization for types that do have getStructInfo method
 *
 * @tparam T
 */
template <typename T>
constexpr bool
    has_getStructInfo<T, std::void_t<decltype(std::declval<T>().getStructInfo())>> = true;

/**
 * @brief returns the getStructInfo method for types that have it,
 * otherwise returns a function that returns an empty StructInfo object.
*/
template<typename T>
std::function<catena::StructInfo()> getTypeFunction() {
  if constexpr (catena::has_getStructInfo<T>) {
    return T::getStructInfo;
  } else {
    return []() -> catena::StructInfo {return catena::StructInfo{};};
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
template <typename T, typename = void>
constexpr bool has_getVariant{};

/**
 * @brief specialization for types that do have getStructInfo method
 *
 * @tparam T
 */
template <typename T>
constexpr bool
    has_getVariant<T, std::void_t<decltype(std::declval<T>().getVariant())>> = true;

/**
 * @brief returns the getStructInfo method for types that have it,
 * otherwise returns a function that returns an empty StructInfo object.
*/
template<typename T>
std::function<catena::StructInfo()> getVariantFunction() {
  if constexpr (catena::has_getVariant<T>) {
    return T::getStructInfo;
  } else {
    return []() -> catena::StructInfo {return catena::StructInfo{};};
  }
}
}  // namespace catena
