#pragma once

/**
 * @file Language.h
 * @brief multi language support in 2 classes: LanguagePack and LanguagePacks
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-08-22
 */

#include <lite/language.pb.h>

#include <string>
#include <unordered_map>
#include <initializer_list>
#include <vector>

namespace catena {
namespace lite {

class LanguagePacks;  // forward declaration

/**
 * @brief a language pack
 */
class LanguagePack {
  public:
    /**
     * @brief a list of words in a language, used by main constructor
     */
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;

    /**
     * @brief Const iterator to the key/word pairs
     */
    using const_iterator = std::unordered_map<std::string, std::string>::const_iterator;

  public:
    LanguagePack() = delete;
    LanguagePack(const LanguagePack&) = default;
    LanguagePack(LanguagePack&&) = default;
    LanguagePack& operator=(const LanguagePack&) = default;
    LanguagePack& operator=(LanguagePack&&) = default;
    virtual ~LanguagePack() = default;

    /**
     * @brief construct a language pack from a list of words
     * @param name the name of the language
     * @param list the list of key/word pairs
     * @param packs the list of language packs
     */
    LanguagePack(const std::string& name, ListInitializer list, LanguagePacks& packs);

    /**
     * @brief deserialize a language pack from a protobuf message
     * @param pack the protobuf message
     */
    inline void fromProto(const catena::LanguagePack& pack);

    /**
     * @brief serialize a language pack to a protobuf message
     * @param pack the protobuf message
     */
    inline void toProto(catena::LanguagePack& pack) const;

    /**
     * get the begin iterator to the key/word pairs
     */
    inline const_iterator begin() const { return words_.cbegin(); }

    /**
     * get the end iterator to the key/word pairs
     */
    inline const_iterator end() const { return words_.cend(); }

  private:
    std::string name_;
    std::unordered_map<std::string, std::string> words_;
};


class LanguagePacks {

  public:
    LanguagePacks() = default;

    /**
     * @brief LanguagePacks does not have copy semantics
     */
    LanguagePacks(const LanguagePacks&) = delete;

    /**
     * @brief LanguagePacks has move semantics
     */
    LanguagePacks(LanguagePacks&&) = default;

    /**
     * @brief LanguagePacks does not have copy semantics
     */
    LanguagePacks& operator=(const LanguagePacks&) = delete;

    /**
     * @brief LanguagePacks has move semantics
     */
    LanguagePacks& operator=(LanguagePacks&&) = delete;

    /**
     * @brief default destructor
     */
    virtual ~LanguagePacks() = default;

    /**
     * @brief add a language to our list of language packs
     */
    void addPack(const std::string& name, LanguagePack* pack);

    /**
     * @brief serialize the list of language names to a protobuf message
     * @param list the protobuf message
     */
    inline void toProto(catena::LanguageList& list) const;

    /**
     * @brief get a language pack by name
     * @param name the name of the language pack
     * @return the language pack, or nullptr if not found
     */
    LanguagePack* getPack(const std::string& name);
    /**
     * @brief serialize language packs to protobuf
     * @param packs the protobuf message
     */
    void toProto(catena::LanguagePacks& packs) const;

  private:
    std::unordered_map<std::string, LanguagePack*> packs_;
};


}  // namespace lite
}  // namespace catena
