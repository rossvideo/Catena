#pragma once

#include "google/protobuf/message_lite.h" 
#include <unordered_map>
#include <string>

namespace catena {
namespace common {
class IPolyglotText {
  public:
    using DisplayStrings = std::unordered_map<std::string, std::string>;
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;
  public:
    IPolyglotText() = default;
    IPolyglotText(IPolyglotText&&) = default;
    IPolyglotText& operator=(IPolyglotText&&) = default;
    virtual ~IPolyglotText() = default;

    virtual void toProto(google::protobuf::MessageLite& msg) const = 0;

    virtual const DisplayStrings& displayStrings() const = 0;

};
}  // namespace common
}  // namespace catena