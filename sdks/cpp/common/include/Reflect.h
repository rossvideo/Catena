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
#include <unordered_map>

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
  fi.getTypeInfo = catena::getTypeFunction<ARGTYPE(x)>();    \
  t.fields.push_back(fi)                                   

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
 * increment 
*/
#define INC(x) PRIMITIVE_CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 10
#define INC_10 11
#define INC_11 12
#define INC_12 13
#define INC_13 14
#define INC_14 15

/**
 * put quotes around a macro argument
 */
#define xstr__(a) str__(a)
#define str__(a) #a

/**
 * @brief adds info about a variant member to the members map in VariantInfo
*/
#define ADD_VARIANT_MEMBER(idx, x) \
vi.members.insert({xstr__(ARGTYPE(x)), {idx, [](void* arg) -> void* {\
       std::variant_alternative_t<idx, vtype> v{}; \
       vtype& dst = *reinterpret_cast<vtype*>(arg); \
       dst = v; \
       return reinterpret_cast<void*>(&std::get<idx>(dst)); \
   }, catena::getTypeFunction<ARGTYPE(x)>()}});

#define DOFOREACH_COUNT(macro, ...) \
  DOFOREACH_COUNT_PASS1(CAT_TOKENS(DOFOREACH_COUNT_, NARGS(__VA_ARGS__)), (macro, __VA_ARGS__))

#define DOFOREACH_COUNT_PASS1(m, x) m x

#define DOFOREACH_COUNT_0(m)
#define DOFOREACH_COUNT_1(m, x1) m(0, x1)
#define DOFOREACH_COUNT_2(m, x1, x2) m(0, x1) m(1, x2)
#define DOFOREACH_COUNT_3(m, x1, x2, x3) m(0, x1) m(1, x2) m(2, x3)
#define DOFOREACH_COUNT_4(m, x1, x2, x3, x4) m(0, x1) m(1, x2) m(2, x3) m(3, x4)
#define DOFOREACH_COUNT_5(m, x1, x2, x3, x4, x5) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(3, x5)
#define DOFOREACH_COUNT_7(m, x1, x2, x3, x4, x5, x6, x7) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7)
#define DOFOREACH_COUNT_8(m, x1, x2, x3, x4, x5, x6, x7, x8) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8)
#define DOFOREACH_COUNT_9(m, x1, x2, x3, x4, x5, x6, x7, x8, x9) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9)
#define DOFOREACH_COUNT_10(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10)
#define DOFOREACH_COUNT_11(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10) m(10, x11)
#define DOFOREACH_COUNT_12(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10) m(10, x11) m(11, x12)
#define DOFOREACH_COUNT_13(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10) m(10, x11) m(11, x12) m(12, x13)
#define DOFOREACH_COUNT_14(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10) m(10, x11) m(11, x12) m(12, x13) m(13, x14)
#define DOFOREACH_COUNT_15(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15) m(0, x1) m(1, x2) m(2, x3) m(3, x4) m(4, x5) m(5, x6) m(6, x7) m(7, x8) m(8, x9) m(9, x10) m(10, x11) m(11, x12) m(12, x13) m(13, x14) m(14, x15)
#define DOFOREACH_COUNT_16(m, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16) \
static_assert(false, "too many arguments to DOFOREACH_COUNT")


/**
 * defines a std::variant type and registers a function to get 
 * runtime info about it
*/
#define REFLECTABLE_VARIANT(classname, ...) \
using classname = std::variant<DOFOREACH(ARGTYPE, __VA_ARGS__)>; \
const catena::VariantInfo CAT_TOKENS(&get, classname)() { \
    using vtype = classname; \
    static catena::VariantInfo vi; \
    if (vi.name.length()) \
        return vi; \
    vi.name = xstr__(classname); \
    DOFOREACH_COUNT(ADD_VARIANT_MEMBER, __VA_ARGS__); \
    return vi; \
} 
}  // namespace catena