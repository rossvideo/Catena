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
//

//common
#include <IParam.h>

//lite
#include <StructInfo.h>

// protobuf interface
#include <interface/param.pb.h>

namespace catena {
namespace lite {

EmptyValue emptyValue;

template<>
void toProto<EmptyValue>(Value& dst, const EmptyValue* src, const AuthzInfo& auth) {
    // do nothing
}

template<>
void fromProto<EmptyValue>(const catena::Value& src, EmptyValue* dst, const AuthzInfo& auth) {
    //do_nothing
}

template<>
void toProto<int32_t>(Value& dst, const int32_t* src, const AuthzInfo& auth) {
    dst.set_int32_value(*src);
}

template<>
void fromProto<int32_t>(const catena::Value& src, int32_t* dst, const AuthzInfo& auth) {
    *dst = src.int32_value();
}

template<>
void toProto<float>(catena::Value& dst, const float* src, const AuthzInfo& auth) {
    dst.set_float32_value(*src);
}

template<>
void fromProto<float>(const catena::Value& src, float* dst, const AuthzInfo& auth) {
    *dst = src.float32_value();
}

template<>
void toProto<std::string>(Value& dst, const std::string* src, const AuthzInfo& auth) {
    dst.set_string_value(*src);
}

template<>
void fromProto<std::string>(const catena::Value& src, std::string* dst, const AuthzInfo& auth) {
    *dst = src.string_value();
}

template<>
void toProto<std::vector<int32_t>>(Value& dst, const std::vector<int32_t>* src, const AuthzInfo& auth) {
    dst.clear_int32_array_values();
    catena::Int32List& int_array = *dst.mutable_int32_array_values();
    for (const int32_t& i : *src) {
        int_array.add_ints(i);
    }
}

template<>
void fromProto<std::vector<int32_t>>(const Value& src, std::vector<int32_t>* dst, const AuthzInfo& auth) {
    dst->clear();
    const catena::Int32List& int_array = src.int32_array_values();
    for (int i = 0; i < int_array.ints_size(); ++i) {
        dst->push_back(int_array.ints(i));
    }
}

template<>
void toProto<std::vector<float>>(Value& dst, const std::vector<float>* src, const AuthzInfo& auth) {
    dst.clear_float32_array_values();
    catena::Float32List& float_array = *dst.mutable_float32_array_values();
    for (const float& f : *src) {
        float_array.add_floats(f);
    }
}

template<>
void fromProto<std::vector<float>>(const Value& src, std::vector<float>* dst, const AuthzInfo& auth) {
    dst->clear();
    const catena::Float32List& float_array = src.float32_array_values();
    for (int i = 0; i < float_array.floats_size(); ++i) {
        dst->push_back(float_array.floats(i));
    }
}

template<>
void toProto<std::vector<std::string>>(Value& dst, const std::vector<std::string>* src, const AuthzInfo& auth) {
    dst.clear_string_array_values();
    catena::StringList& string_array = *dst.mutable_string_array_values();
    for (const std::string& s :*src) {
        string_array.add_strings(s);
    }
}

template<>
void fromProto<std::vector<std::string>>(const Value& src, std::vector<std::string>* dst, const AuthzInfo& auth) {
    dst->clear();
    const catena::StringList& string_array = src.string_array_values();
    for (int i = 0; i < string_array.strings_size(); ++i) {
        dst->push_back(string_array.strings(i));
    }
}

} // namespace lite
} // namespace catena
