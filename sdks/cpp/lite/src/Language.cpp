#include <lite/include/Language.h>


using namespace ::catena::lite;

LanguagePack::LanguagePack(const std::string& name, ListInitializer list, LanguagePacks& packs)
    : name_{name}, words_(list.begin(), list.end()) {
    packs.addPack(name, this);
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

void LanguagePacks::addPack(const std::string& name, LanguagePack* pack) { packs_[name] = pack; }

LanguagePack* LanguagePacks::getPack(const std::string& name) {
    auto it = packs_.find(name);
    if (it != packs_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

void LanguagePacks::toProto(catena::LanguagePacks& packs) const {
    packs.clear_packs();
    auto& proto_packs = *packs.mutable_packs();
    for (const auto& [name, pack] : packs_) {
        proto_packs[name].set_name(name);
        auto& words = *proto_packs[name].mutable_words();
        words.insert(pack->begin(), pack->end());
    }
}

void LanguagePacks::toProto(catena::LanguageList& list) const {
    list.clear_languages();
    for (const auto& [name, pack] : packs_) {
        list.add_languages(name);
    }
}
