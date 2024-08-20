#include <lite/include/PolyglotText.h>

#include <string>
#include <unordered_map>

namespace catena {
namespace lite {
void PolyglotText::toProto(google::protobuf::MessageLite& m) const {
    auto& dst = dynamic_cast<catena::PolyglotText&>(m);
    dst.clear_display_strings();
    for (const auto& [key, value] : display_strings_) {
        dst.mutable_display_strings()->insert({key, value});
    }
}
}  // namespace lite
}  // namespace catena