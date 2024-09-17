// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
//using catena::lite::StructInfo;
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



