#include <lite/include/Param.h>
#include <lite/param.pb.h>

using catena::Value;
using catena::lite::Param;

template <>
void Param<std::string>::serialize(Value& value) const {
    *value.mutable_string_value() = value_.get();
}
