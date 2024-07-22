#pragma once

#include <IPolyglotText.h>

#include <lite/language.pb.h>

#include <string>
#include <unordered_map>
#include <initializer_list>

namespace catena {
namespace lite {
class PolyglotText : public catena::common::IPolyglotText {
  public:
    using DisplayStrings = std::unordered_map<std::string, std::string>;

  public:
    PolyglotText(const DisplayStrings& display_strings) : display_strings_(display_strings) {}
    PolyglotText() = default;
    PolyglotText(PolyglotText&&) = default;
    PolyglotText& operator=(PolyglotText&&) = default;
    virtual ~PolyglotText() = default;

    // Constructor from initializer list
    PolyglotText(std::initializer_list<std::pair<const std::string, std::string>> list)
      : display_strings_(list.begin(), list.end()) {}


    void toProto(google::protobuf::MessageLite& dst) const override;

    inline const DisplayStrings& displayStrings() const override { return display_strings_; }

  private:
    DisplayStrings display_strings_;
};
}  // namespace lite
}  // namespace catena