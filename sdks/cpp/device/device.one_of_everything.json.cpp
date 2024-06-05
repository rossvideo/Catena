// This file was auto-generated. Do not modify by hand.
#include <device.one_of_everything.json.h>
const StructInfo& one_of_everything::Inner_nested_struct::getStructInfo() {
  static StructInfo t;
  if (t.name.length()) return t;
  t.name = "Inner_nested_struct";
  catena::FieldInfo fi;
    // register info for the num_1 field
    //
    fi.name = "num_1";
    fi.offset = offsetof(one_of_everything::Inner_nested_struct, num_1);
    fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) {
      auto dst = reinterpret_cast<int32_t*>(dstAddr);
      pa->getValue<false, int32_t>(*dst);
    };
    fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) {
      auto src = reinterpret_cast<const int32_t*>(srcAddr);
      pa->setValue<false, int32_t>(*src);
    };
    t.fields.push_back(fi);
    // register info for the num_2 field
    //
    fi.name = "num_2";
    fi.offset = offsetof(one_of_everything::Inner_nested_struct, num_2);
    fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) {
      auto dst = reinterpret_cast<int32_t*>(dstAddr);
      pa->getValue<false, int32_t>(*dst);
    };
    fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) {
      auto src = reinterpret_cast<const int32_t*>(srcAddr);
      pa->setValue<false, int32_t>(*src);
    };
    t.fields.push_back(fi);
  return t;
}
const StructInfo& one_of_everything::Outer_nested_struct::getStructInfo() {
  static StructInfo t;
  if (t.name.length()) return t;
  t.name = "outer_nested_struct";
  catena::FieldInfo fi;
    // register info for the inner_nested_struct field
    //
    fi.name = "inner_nested_struct";
    fi.offset = offsetof(one_of_everything::Outer_nested_struct, inner_nested_struct);
    fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) {
      auto dst = reinterpret_cast<Inner_nested_struct*>(dstAddr);
      pa->getValue<false, Inner_nested_struct>(*dst);
    };
    fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) {
      auto src = reinterpret_cast<const Inner_nested_struct*>(srcAddr);
      pa->setValue<false, Inner_nested_struct>(*src);
    };
    t.fields.push_back(fi);
    // register info for the text field
    //
    fi.name = "text";
    fi.offset = offsetof(one_of_everything::Outer_nested_struct, text);
    fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) {
      auto dst = reinterpret_cast<std::string*>(dstAddr);
      pa->getValue<false, std::string>(*dst);
    };
    fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) {
      auto src = reinterpret_cast<const std::string*>(srcAddr);
      pa->setValue<false, std::string>(*src);
    };
    t.fields.push_back(fi);
  return t;
}
