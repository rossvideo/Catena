#pragma once

/**THIS IS A COPY OF LanguagePack.h NOT READY FOR RELEASE */

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

/**
 * @file MenuGroup.h
 * @brief MenuGroups support
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

//common
#include <IMenuGroup.h>

// lite
#include <Device.h>

// protobuf interface
#include <interface/MenuGroup.pb.h> //Unsure where to create this file

#include <string>
#include <unordered_map>
#include <initializer_list>
#include <vector>

namespace catena {
namespace lite {

class MenuGroup;  // forward declaration

/**
 * @brief MenuGroup list
 */
class MenuGroup : public common::IMenuGroup {
  public:
    /**
     * @brief MenuGroup list initializer
     */
    using ListInitializer = std::initializer_list<std::pair<std::string, std::string>>;

  public:
    MenuGroup() = delete;
    /**
     * @brief MenuGroup does not have copy semantics
     */
    MenuGroup(const MenuGroup&) = delete;

    /**
     * @brief MenuGroup has move semantics
     */
    MenuGroup(MenuGroup&&) = default;

    /**
     * @brief MenuGroup does not have copy semantics
     */
    MenuGroup& operator=(const MenuGroup&) = delete;

    /**
     * @brief MenuGroup has move semantics
     */
    MenuGroup& operator=(MenuGroup&&) = default;

    /**
     * destructor
     */
    virtual ~MenuGroup() = default;

    /**
     * @brief construct a language pack from a list of words
     * @param name the name of the language
     * @param list the list of key/word pairs
     * @param dev the device model to which this language pack belongs
     */
    MenuGroup(const std::string& name, ListInitializer list, Device& dev);

    /**
     * @brief deserialize a language pack from a protobuf message
     * @param pack the protobuf message
     */
    void fromProto(const ::catena::MenuGroup& pack) override;

    /**
     * @brief serialize a language pack to a protobuf message
     * @param pack the protobuf message
     */
    void toProto(::catena::MenuGroup& pack) const override;

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
