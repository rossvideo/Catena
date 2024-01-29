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
struct TypeInfo {
  std::string name;              /**< the data structure's name */
  std::vector<FieldInfo> fields; /**< name and offset info per field */
};

// class TypeTraits;  // forward reference

/**
 * @brief holds name and value information about a stucture's field.
 *
 */
struct FieldInfo {
  std::string name; /**< the field's name */
  int offset;       /**< the offset to the field's data from struct base */
  std::function<TypeInfo()> getTypeInfo; /**< type info of nested struct */
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
  std::function<TypeInfo()> getTypeInfo; /**< type info of nested struct */
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
constexpr bool has_getType{};

/**
 * @brief specialization for types that do have getType method
 *
 * @tparam T
 */
template <typename T>
constexpr bool
    has_getType<T, std::void_t<decltype(std::declval<T>().getType())>> = true;

/**
 * @brief returns the getType method for types that have it,
 * otherwise returns a function that returns an empty TypeInfo object.
*/
template<typename T>
std::function<catena::TypeInfo()> getTypeFunction() {
  if constexpr (catena::has_getType<T>) {
    return T::getType;
  } else {
    return []() -> catena::TypeInfo {return catena::TypeInfo{};};
  }
}


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
constexpr bool has_getVariant{};

/**
 * @brief specialization for types that do have getType method
 *
 * @tparam T
 */
template <typename T>
constexpr bool
    has_getVariant<T, std::void_t<decltype(std::declval<T>().getVariant())>> = true;

/**
 * @brief returns the getType method for types that have it,
 * otherwise returns a function that returns an empty TypeInfo object.
*/
template<typename T>
std::function<catena::TypeInfo()> getVariantFunction() {
  if constexpr (catena::has_getVariant<T>) {
    return T::getType;
  } else {
    return []() -> catena::TypeInfo {return catena::TypeInfo{};};
  }
}
// /**
//  * @brief base class providing conversion interface.
//  *
//  */
// class TypeTraits {
//  public:
//   virtual const std::type_info& runtimeType() const { return typeid(void); }

//   /**
//    * @brief Get the Array Element Type object
//    *
//    * @param type info about the type
//    * @return false unless overridden
//    */
//   virtual bool getArrayElementType(catena::TypeInfo*& type) {
//     // By default, not an array type
//     return false;
//   }

//   /**
//    * @brief override to return size of array fields
//    *
//    * @param p points to array.
//    * @return size_t size of the array
//    */
//   virtual size_t arraySize(void* p) { return 0; }

//   /**
//    * @brief Set the Array Size object
//    *
//    * @param p points to the array
//    * @param size to set
//    */
//   virtual void setArraySize(void* p, size_t size) {}

//   /**
//    * @brief gets array element
//    *
//    * @param p base of array
//    * @param i index of element
//    * @return void* pointer to element
//    */
//   virtual void* arrayElement(void* p, size_t i) {
//     return nullptr;  // Invalid operation, since not array
//   }

//   /**
//    * @brief convert field to string, override with templated function.
//    *
//    * @param pField points to field data
//    * @return std::string field data represented as string
//    */
//   virtual std::string toString(void* pField) { return std::string(); }

//   /**
//    * @brief override to convert from string to field data
//    *
//    * @param pField points to field to write
//    * @param value string represenation of field value
//    */
//   virtual void fromString(void* pField, const std::string& value) {}

//   /**
//    * @brief override to create a properly initialized ParamType
//    *
//    */
//   virtual std::unique_ptr<ParamType> paramType() {
//     return std::make_unique<ParamType>();
//   }

//   /**
//    * @brief override to read a field's value as a catena::Param.Value object
//    *
//    * @param pField pointer to the field's value
//    */
//   virtual std::unique_ptr<Value> paramValue(const void* pField) {
//     return std::make_unique<Value>();
//   }

//   virtual void setValue(void* pField, const Value&) {}
// };

// /**
//  * @brief Generic class definition, which can be applied to any class type.
//  *
//  * @tparam T
//  */
// template <typename T>
// class TypeTraitsT : public TypeTraits {
//  public:
//   /**
//    * @brief specialization for reflected structs
//    *
//    * @return properly initialized struct_type
//    */
//   typename std::enable_if<has_getType<T>, std::unique_ptr<ParamType>>::type
//   paramType() override {
//     std::unique_ptr<ParamType> struct_type(new ParamType);
//     // catena::ParamDescriptors* descs = struct_type->mutable_struct_type();
//     // catena::TypeInfo info = T::getType();
//     // for (auto it = info.fields.begin(); it != info.fields.end(); ++it) {
//     //   catena::Param sub_param{};
//     //   *sub_param.mutable_type() = *it->fieldType->paramType();
//     //   (*descs->mutable_descriptors())[it->name] = sub_param;
//     // }
//     return struct_type;
//   }

//   /**
//    * @brief specialization for reflected structs
//    *
//    * @return parameter value
//    *
//    */
//   typename std::enable_if<has_getType<T>, std::unique_ptr<Value>>::type
//   paramValue(const void* base) override {
//     std::unique_ptr<Value> ans(new Value);
//     // StructValue struct_value;
//     // catena::TypeInfo info = T::getType();
//     // for (auto it = info.fields.begin(); it != info.fields.end(); ++it) {
//     //   auto field_value = it->fieldType->paramValue(
//     //       static_cast<const char*>(base) + it->offset);
//     //   (*struct_value.mutable_fields())[it->name] = *field_value;
//     // }
//     // *ans->mutable_struct_value() = struct_value;
//     return ans;
//   }

//   /**
//    * @brief Set the Value object
//    *
//    * @param base
//    * @param p
//    * @todo add has_* validation tests, current code assumes Param has
//    * a value, and that it's a struct value
//    *
//    */
//   typename std::enable_if<has_getType<T>, void>::type setValue(
//       void* base, const Value& v) override {
//     // StructValue sv = v.struct_value();
//     // catena::TypeInfo info = T::getType();
//     // for (auto it = info.fields.begin(); it != info.fields.end(); ++it) {
//     //   char* pField = static_cast<char*>(base) + it->offset;
//     //   const Value& val = sv.fields().at(it->name);
//     //   it->fieldType->setValue(pField, val);
//     // }
//   }
// };

// // template <>
// // class TypeTraitsT<std::string> : public TypeTraits {
// //  public:
// //   const std::type_info& runtimeType() const override {
// //     return typeid(std::string);
// //   }
// //   std::string toString(void* pField) override {
// //     std::string* ps = static_cast<std::string*>(pField);
// //     return *ps;
// //   }

// //   void fromString(void* pField, const std::string& value) override {
// //     *reinterpret_cast<std::string*>(pField) = value;
// //   }

// //   std::unique_ptr<ParamType> paramType() override {
// //     std::unique_ptr<ParamType> string_type(new ParamType);
// //     string_type->set_simple_type(catena::ParamType_SimpleTypes_STRING);
// //     return string_type;
// //   }

// //   std::unique_ptr<Value> paramValue(const void* pField) override {
// //     std::unique_ptr<Value> string_value(new Value);
// //     *string_value->mutable_string_value() =
// //         *reinterpret_cast<const std::string*>(pField);
// //     return string_value;
// //   }

// //   void setValue(void* pField, const Value& v) override {
// //     *reinterpret_cast<std::string*>(pField) = v.string_value();
// //   }
// // };

// // template <>
// // class TypeTraitsT<int> : public TypeTraits {
// //  public:
// //   std::string toString(void* pField) override {
// //     return std::to_string(*reinterpret_cast<int*>(pField));
// //   }

// //   void fromString(void* pField, const std::string& value) override {
// //     std::size_t pos;
// //     std::stoi(*reinterpret_cast<std::string*>(pField), &pos);
// //   }

// //   std::unique_ptr<ParamType> paramType() {
// //     std::unique_ptr<ParamType> int_type(new ParamType);
// //     int_type->set_simple_type(catena::ParamType_SimpleTypes_STRING);
// //     return int_type;
// //   }
// // };

// template <>
// class TypeTraitsT<float> : public TypeTraits {
//  public:
//   const std::type_info& runtimeType() const override { return typeid(float); }
//   std::string toString(void* pField) override {
//     return std::to_string(*reinterpret_cast<float*>(pField));
//   }

//   void fromString(void* pField, const std::string& value) override {
//     std::size_t pos;
//     std::stof(*reinterpret_cast<std::string*>(pField), &pos);
//   }

//   std::unique_ptr<ParamType> paramType() override {
//     std::unique_ptr<ParamType> float_type(new ParamType);
//     // float_type->set_simple_type(catena::ParamType_SimpleTypes_FLOAT);
//     return float_type;
//   }

//   std::unique_ptr<Value> paramValue(const void* pField) override {
//     std::unique_ptr<Value> float_value(new Value);
//     // float_value->set_float_value(*reinterpret_cast<const float*>(pField));
//     return float_value;
//   }

//   void setValue(void* pField, const Value& v) override {
//     // *reinterpret_cast<float*>(pField) = v.float_value();
//   }
// };

// template <>
// class TypeTraitsT<int32_t> : public TypeTraits {
//  public:
//   const std::type_info& runtimeType() const override { return typeid(int32_t); }
//   std::string toString(void* pField) override {
//     return std::to_string(*reinterpret_cast<int32_t*>(pField));
//   }

//   void fromString(void* pField, const std::string& value) override {
//     std::size_t pos;
//     std::stoi(*reinterpret_cast<std::string*>(pField), &pos);
//   }

//   std::unique_ptr<ParamType> paramType() override {
//     std::unique_ptr<ParamType> int32_type(new ParamType);
//     // float_type->set_simple_type(catena::ParamType_SimpleTypes_FLOAT);
//     return int32_type;
//   }

//   std::unique_ptr<Value> paramValue(const void* pField) override {
//     std::unique_ptr<Value> int32_value(new Value);
//     // float_value->set_float_value(*reinterpret_cast<const float*>(pField));
//     return int32_value;
//   }

//   void setValue(void* pField, const Value& v) override {
//     // *reinterpret_cast<float*>(pField) = v.float_value();
//   }
// };

// // template <>
// // class TypeTraitsT<bool> : public TypeTraits {
// //  public:
// //   std::string toString(void* p) override {
// //     return *reinterpret_cast<bool*>(p) ? std::string("true")
// //                                        : std::string("false");
// //   }

// //   void fromString(void* pField, const std::string& value) override {
// //     bool* b = reinterpret_cast<bool*>(pField);
// //     *b = false;
// //     if (value == std::string("true")) {
// //       *b = true;
// //     }
// //   }

// //   /**
// //    * @brief apply bool type, but as an INT32
// //    *
// //    * @return std::unique_ptr<ParamType>
// //    */
// //   std::unique_ptr<ParamType> paramType() {
// //     std::unique_ptr<ParamType> bool_type(new ParamType);
// //     bool_type->set_simple_type(catena::ParamType_SimpleTypes_INT32);
// //     return bool_type;
// //   }
// // };

// // template <typename T>
// // class TypeTraitsT<std::vector<T>> : public TypeTraits {
// //  public:
// //   using vec_type = std::vector<T>;
// //   bool getArrayElementType(TypeInfo*& type) override {
// //     if constexpr (has_getType<T>) {
// //       type = &T::getType();
// //     }
// //     return true;
// //   }

// //   size_t arraySize(void* p) override {
// //     auto v = reinterpret_cast<vec_type*>(p);
// //     return v->size();
// //   }

// //   void setArraySize(void* p, size_t size) override {
// //     auto v = reinterpret_cast<vec_type*>(p);
// //     v->resize(size);
// //   }

// //   void* arrayElement(void* p, size_t i) override {
// //     auto v = reinterpret_cast<vec_type*>(p);
// //     return &v->at(i);
// //   }

// //   std::string toString(void* pField) override {
// //     std::stringstream ans;
// //     size_t sz = arraySize(pField);
// //     for (size_t idx = 0; idx < sz; ++idx) {
// //       auto pElement = reinterpret_cast<T*>(arrayElement(pField, idx));
// //       ans << *pElement;
// //       if (idx != sz - 1) {
// //         ans << ", ";
// //       }
// //     }
// //     return ans.str();
// //   }
// // };

}  // namespace catena
