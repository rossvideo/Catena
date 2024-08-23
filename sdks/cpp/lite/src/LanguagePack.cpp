#include <lite/include/LanguagePack.h>


using namespace catena::lite;
using catena::common::LanguagePackTag;

LanguagePack::LanguagePack(const std::string& name, ListInitializer list, Device& dev)
    : name_{name}, words_(list.begin(), list.end()) {
    dev.addItem<LanguagePackTag>(name, this);
}

void LanguagePack::fromProto(const catena::LanguagePack& pack) {
    name_ = pack.name();
    for (const auto& [key, value] : pack.words()) {
        words_[key] = value;
    }
}

void LanguagePack::toProto(catena::LanguagePack& pack) const {
    pack.set_name(name_);
    for (const auto& [key, value] : words_) {
        (*pack.mutable_words())[key] = value;
    }
}



