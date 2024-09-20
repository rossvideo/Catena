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

//lite
#include <StructInfo.h>

// protobuf interface
#include <interface/param.pb.h>


template<>
void catena::lite::toProto<float>(catena::Value& dst, const void* src) {
    dst.set_float32_value(*reinterpret_cast<const float*>(src));
}

template<>
void catena::lite::fromProto<float>(void* dst, const catena::Value& src) {
    *reinterpret_cast<float*>(dst) = src.float32_value();
}

template<>
void catena::lite::toProto<int32_t>(Value& dst, const void* src) {
    dst.set_int32_value(*reinterpret_cast<const int32_t*>(src));
}

template<>
void catena::lite::fromProto<int32_t>(void* dst, const catena::Value& src) {
    *reinterpret_cast<int32_t*>(dst) = src.int32_value();
}

template<>
void catena::lite::toProto<std::string>(Value& dst, const void* src) {
    *dst.mutable_string_value() = *reinterpret_cast<const std::string*>(src);
}

template<>
void catena::lite::fromProto<std::string>(void* dst, const catena::Value& src) {
    *reinterpret_cast<std::string*>(dst) = src.string_value();
}

template<>
void catena::lite::toProto<std::vector<std::string>>(Value& dst, const void* src) {
    dst.clear_string_array_values();
    auto& string_array = *dst.mutable_string_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<std::string>*>(src);
    for (const auto& s : vec) {
        string_array.add_strings(s);
    }
}

template<>
void catena::lite::fromProto<std::vector<std::string>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<std::string>*>(dst);
    vec->clear();
    const auto& string_array = src.string_array_values();
    for (int i = 0; i < string_array.strings_size(); ++i) {
        vec->push_back(string_array.strings(i));
    }
}

template<>
void catena::lite::toProto<std::vector<int32_t>>(Value& dst, const void* src) {
    dst.clear_int32_array_values();
    auto& int_array = *dst.mutable_int32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<int32_t>*>(src);
    for (const auto& i : vec) {
        int_array.add_ints(i);
    }
}

template<>
void catena::lite::fromProto<std::vector<int32_t>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<int32_t>*>(dst);
    vec->clear();
    const auto& int_array = src.int32_array_values();
    for (int i = 0; i < int_array.ints_size(); ++i) {
        vec->push_back(int_array.ints(i));
    }
}

template<>
void catena::lite::toProto<std::vector<float>>(Value& dst, const void* src) {
    dst.clear_float32_array_values();
    auto& float_array = *dst.mutable_float32_array_values();
    const auto& vec = *reinterpret_cast<const std::vector<float>*>(src);
    for (const auto& f : vec) {
        float_array.add_floats(f);
    }
}

template<>
void catena::lite::fromProto<std::vector<float>>(void* dst, const Value& src) {
    auto* vec = reinterpret_cast<std::vector<float>*>(dst);
    vec->clear();
    const auto& float_array = src.float32_array_values();
    for (int i = 0; i < float_array.floats_size(); ++i) {
        vec->push_back(float_array.floats(i));
    }
}
