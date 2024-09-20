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

// lite
#include <ParamDescriptor.h>
#include <StructInfo.h>

// protobuf interface
#include <interface/param.pb.h>

#include <vector>
#include <string>
#include <type_traits>


using catena::Value;
using catena::lite::ParamDescriptor;
using catena::lite::StructInfo;
using catena::lite::FieldInfo;


// template <>
// void ParamDescriptor<int32_t>::toProto(Value& value) const {
//     catena::lite::toProto<int32_t>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<int32_t>::fromProto(const Value& value) {
//     catena::lite::fromProto<int32_t>(&value_.get(), value);
// }

// template <>
// void ParamDescriptor<std::string>::toProto(Value& value) const {
//     catena::lite::toProto<std::string>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<std::string>::fromProto(const Value& value) {
//     catena::lite::fromProto<std::string>(&value_.get(), value);
// }

// template <>
// void ParamDescriptor<float>::toProto(Value& value) const {
//     catena::lite::toProto<float>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<float>::fromProto(const Value& value) {
//     catena::lite::fromProto<float>(&value_.get(), value);
// }

// template <>
// void ParamDescriptor<std::vector<std::string>>::toProto(Value& value) const {
//     catena::lite::toProto<std::vector<std::string>>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<std::vector<std::string>>::fromProto(const Value& value) {
//     catena::lite::fromProto<std::vector<std::string>>(&value_.get(), value);
// }

// template <>
// void ParamDescriptor<std::vector<std::int32_t>>::toProto(Value& value) const {
//     catena::lite::toProto<std::vector<std::int32_t>>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<std::vector<std::int32_t>>::fromProto(const Value& value) {
//     catena::lite::fromProto<std::vector<std::int32_t>>(&value_.get(), value);
// }

// template <>
// void ParamDescriptor<std::vector<float>>::toProto(Value& value) const {
//     catena::lite::toProto<std::vector<float>>(value, &value_.get());
// }

// template <>
// void ParamDescriptor<std::vector<float>>::fromProto(const Value& value) {
//     catena::lite::fromProto<std::vector<float>>(&value_.get(), value);
// }



