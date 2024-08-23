#pragma once

/**
 * @file LanguagePack.h
 * @brief multi language support in 2 classes: LanguagePack and LanguagePacks
 * @author John R. Naylor john.naylor@rossvideo.com
 * @date 2024-08-22
 */

#include <lite/language.pb.h>

#include <lite/include/Device.h>
#include <common/include/ILanguagePack.h>

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
class LanguagePack : public common::ILanguagePack {
  public:
    /**
     * @brief a list of words in a language, used by main constructor
     */
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;

  public:
    LanguagePack() = delete;
    /**
     * @brief LanguagePack does not have copy semantics
     */
    LanguagePack(const LanguagePack&) = delete;

    /**
     * @brief LanguagePack has move semantics
     */
    LanguagePack(LanguagePack&&) = default;

    /**
     * @brief LanguagePack does not have copy semantics
     */
    LanguagePack& operator=(const LanguagePack&) = delete;

    /**
     * @brief LanguagePack has move semantics
     */
    LanguagePack& operator=(LanguagePack&&) = default;

    /**
     * destructor
     */
    virtual ~LanguagePack() = default;

    /**
     * @brief construct a language pack from a list of words
     * @param name the name of the language
     * @param list the list of key/word pairs
     * @param dev the device model to which this language pack belongs
     */
    LanguagePack(const std::string& name, ListInitializer list, Device& dev);

    /**
     * @brief deserialize a language pack from a protobuf message
     * @param pack the protobuf message
     */
    void fromProto(const ::catena::LanguagePack& pack) override;

    /**
     * @brief serialize a language pack to a protobuf message
     * @param pack the protobuf message
     */
    void toProto(::catena::LanguagePack& pack) const override;

    /**
     * get the begin iterator to the key/word pairs
     */
    inline const_iterator begin() const override{ return words_.cbegin(); }

    /**
     * get the end iterator to the key/word pairs
     */
    inline const_iterator end() const override { return words_.cend(); }

  private:
    std::string name_;
    std::unordered_map<std::string, std::string> words_;
};


}  // namespace lite
}  // namespace catena
