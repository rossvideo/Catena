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
#include <Meta.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>


using catena::meta::TypeList;
using catena::meta::NthElement;

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
#define PUSH_FIELD_INFO(idx, x)                              \
  fi.setName(ARGNAME_AS_STRING(x));                          \
  fi.offset = offsetof(_className, ARGNAME(x));              \
  fi.getStructInfo = catena::getStructInfoFunction<ARGTYPE(x)>();    \
  fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) { \
    auto dst = reinterpret_cast<NthElement<typelist, idx>*>(dstAddr); \
    pa->getValueNative<false, NthElement<typelist, idx>>(*dst); \
  }; \
  fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) { \
    auto src = reinterpret_cast<const NthElement<typelist, idx>*>(srcAddr); \
    pa->setValueNative<false, NthElement<typelist, idx>>(*src); \
  }; \
  t.fields.push_back(fi);                               

/**
 * @brief makes a struct reflectable by initializing a StructInfo object.
 *
 * The StructInfo has static storage
 *
 * Type conversion is defined by TypeTraitsT
 *
 */
#define REFLECTABLE_STRUCT(className, ...)                   \
  /* Dump field types and names */                    \
  DOFOREACH_SEMICOLON(ARGPAIR, __VA_ARGS__)           \
  /* accessible from PUSH_FIELD_INFO define */        \
  using typelist = TypeList< DOFOREACH(ARGTYPE, __VA_ARGS__) >; \
  using _className = className;                       \
  static const catena::StructInfo& getStructInfo() {  \
    static catena::StructInfo t;                        \
    if (t.name.length()) return t;                    \
    t.name = #className;                              \
    catena::FieldInfo fi;                             \
    /* Dump offsets and field names */                \
    DOFOREACH_COUNT_SEMICOLON(PUSH_FIELD_INFO, __VA_ARGS__) \
    return t;                                         \
  }

/**
 * @brief adds info about a variant member to the members map in VariantInfo
*/
#define ADD_VARIANT_MEMBER(idx, x) \
vi.lookup.push_back(QUOTED(ARGTYPE(x))); \
vi.members.insert({QUOTED(ARGTYPE(x)), \
  { \
    idx, \
    [](void* arg) -> void* { \
      vtype& dst = *reinterpret_cast<vtype*>(arg); \
      if (dst.index() != idx) { \
        /* change the variant type to the new type */ \
        std::variant_alternative_t<idx, vtype> v{}; \
        dst = v; \
      } \
      return reinterpret_cast<void*>(&std::get<idx>(dst)); \
    }, \
    catena::getStructInfoFunction<ARGTYPE(x)>(), \
    [](void* dstAddr, const ParamAccessor* pa) { \
      auto dst = reinterpret_cast<std::variant_alternative_t<idx, vtype>*>(dstAddr); \
      pa->getValueNative<false, std::variant_alternative_t<idx, vtype>>(*dst); \
    }, \
    [](ParamAccessor* pa, const void* srcAddr) { \
      auto src = reinterpret_cast<const std::variant_alternative_t<idx, vtype>*>(srcAddr); \
      pa->setValueNative<false, std::variant_alternative_t<idx, vtype>>(*src); \
    } \
}});



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
    vi.name = QUOTED(classname); \
    DOFOREACH_COUNT(ADD_VARIANT_MEMBER, __VA_ARGS__); \
    return vi; \
} \
auto& CAT_TOKENS(classname, _funct) = catena::ParamAccessor::VariantInfoGetter::getInstance(); \
bool CAT_TOKENS(classname, _added) = CAT_TOKENS(classname, _funct).addFunction(std::type_index(typeid(classname)),CAT_TOKENS(get, classname))


}  // namespace catena