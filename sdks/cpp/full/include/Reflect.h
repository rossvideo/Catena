#pragma once

/**
 * @file Reflect.h
 * @brief adapted from https://github.com/tapika/TestCppReflect
 *
 * @copyright not asserted
 * license https://github.com/tapika/TestCppReflect/blob/master/license.txt
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
#include <ReflectionMacros.h>
#include <TypeTraits.h>
#include <TypeList.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>


namespace catena {
  namespace full {

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
    auto dst = reinterpret_cast<catena::meta::NthElement<typelist, idx>*>(dstAddr); \
    pa->getValue<false, catena::meta::NthElement<typelist, idx>>(*dst); \
  }; \
  fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) { \
    auto src = reinterpret_cast<const catena::meta::NthElement<typelist, idx>*>(srcAddr); \
    pa->setValue<false, catena::meta::NthElement<typelist, idx>>(*src); \
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
  using typelist = catena::meta::Typelist< DOFOREACH(ARGTYPE, __VA_ARGS__) >; \
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
      pa->getValue<false, std::variant_alternative_t<idx, vtype>>(*dst); \
    }, \
    [](ParamAccessor* pa, const void* srcAddr) { \
      auto src = reinterpret_cast<const std::variant_alternative_t<idx, vtype>*>(srcAddr); \
      pa->setValue<false, std::variant_alternative_t<idx, vtype>>(*src); \
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

  }  // namespace full
}  // namespace catena