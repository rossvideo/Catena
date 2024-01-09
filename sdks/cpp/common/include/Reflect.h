#pragma once

/**
 * @file Reflect.h
 * @brief adapted from https://github.com/tapika/TestCppReflect
 *
 * @copyright not asserted
 * license https://github.com/tapika/TestCppReflect/blob/master/license.txt
 *
 */
#include <ReflectionMacros.h>
#include <TypeTraits.h>

#include <memory>
#include <string>
#include <vector>

namespace catena {

/**
 * @brief pushes the field info
 *
 * NB uses offsetof which isn't very portable.
 * https://en.cppreference.com/w/cpp/types/offsetof
 *
 * Works in gcc (tested) and LLVM (not tested)
 *
 */
#define PUSH_FIELD_INFO(x)                                   \
  fi.setName(ARGNAME_AS_STRING(x));                          \
  fi.offset = offsetof(_className, ARGNAME(x));              \
  fi.fieldType.reset(new catena::TypeTraitsT<ARGTYPE(x)>()); \
  t.fields.push_back(fi);

/**
 * @brief makes a struct reflectable by initializing a TypeInfo object.
 *
 * The TypeInfo has static storage
 *
 * Type conversion is defined by TypeTraitsT
 *
 */
#define REFLECTABLE(className, ...)                   \
  /* Dump field types and names */                    \
  DOFOREACH_SEMICOLON(ARGPAIR, __VA_ARGS__)           \
  /* accessible from PUSH_FIELD_INFO define */        \
  using _className = className;                       \
  static const catena::TypeInfo& getType() {          \
    static catena::TypeInfo t;                        \
    if (t.name.length()) return t;                    \
    t.name = #className;                              \
    catena::FieldInfo fi;                             \
    /* Dump offsets and field names */                \
    DOFOREACH_SEMICOLON(PUSH_FIELD_INFO, __VA_ARGS__) \
    return t;                                         \
  }

/**
 * @brief applies the type T to the parameter
 *
 * @tparam T a REFLECTED struct
 * @param p the parameter to apply the struct type to
 */
template <typename T>
static void applyType(catena::Param& p) {
  *p.mutable_type() = *TypeTraitsT<T>().paramType();
}

/**
 * @brief Set the Value object
 *
 * @tparam T
 * @param p protobuf Param to set the value of
 * @param value value to use
 */
template <typename T>
static void setValue(catena::Param& p, const T& value) {
  *p.mutable_value() = *TypeTraitsT<T>().paramValue(&value);
}

/**
 * @brief Get the value from a catena::Param
 *
 * @tparam T type of object to write to
 * @param value to update
 * @param p param object to read
 */
template <typename T>
static void getValue(T& value, const catena::Param& p) {
  TypeTraitsT<T>().setValue(&value, p.value());
}
}  // namespace catena
