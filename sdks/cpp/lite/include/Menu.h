#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.
*/
//

/**
 * @file Menu.h
 * @brief Menus support
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IMenu.h>

// lite
#include <Device.h>
#include <MenuGroup.h>

// protobuf interface
#include <interface/menu.pb.h>

#include <string>
#include <unordered_map>
#include <vector>


// Forward declaration of MenuGroup
namespace catena {
namespace lite {
class MenuGroup;
} // namespace lite
} // namespace catena

namespace catena {
namespace lite {

class Menu;  // forward declaration

/**
 * @brief A device menu
 */
class Menu : public common::IMenu {
  public:
    /**
     * @brief a list of oids, used by main constructor
     */
    using OidInitializer = std::initializer_list<std::string>;

    /**
     * @brief a list of string pairs, used by main constructor
     */
    using PairInitializer = std::initializer_list<std::pair<std::string, std::string>>;

  public:
    Menu() = delete;
    /**
     * @brief Menu does not have copy semantics
     */
    Menu(const Menu&) = delete;

    /**
     * @brief Menu has move semantics
     */
    Menu(Menu&&) = default;

    /**
     * @brief Menu does not have copy semantics
     */
    Menu& operator=(const Menu&) = delete;

    /**
     * @brief Menu has move semantics
     */
    Menu& operator=(Menu&&) = default;

    /**
     * destructor
     */
    virtual ~Menu() = default;

    /**
     * @brief construct a menu item
     * @param name the name of the menu
     * @param hidden whether the menu is hidden
     * @param disabled whether the menu is disabled
     * @param param_oids The menu's parameter members
     * @param comand_oids The menu's command members
     * @param client_hints map of client hints
     * @param oid the oid of the menu
     * @param menuGroup the menu group to add the menu to
     */
    Menu(const PolyglotText& name, bool hidden, bool disabled,
         const OidInitializer param_oids,
         const OidInitializer command_oids,
         const PairInitializer& client_hints, 
         std::string oid, catena::lite::MenuGroup& menuGroup);

    /**
     * @brief deserialize a menu from a protobuf message
     * @param menu the protobuf message
     */
    void fromProto(const ::catena::Menu& menu) override;

    /**
     * @brief serialize a menu to a protobuf message
     * @param menu the protobuf message
     */
    void toProto(::catena::Menu& menu) const override;

  private:
    PolyglotText name_;
    bool hidden_;
    bool disabled_;
    std::vector<std::string> param_oids_;
    std::vector<std::string> command_oids_;
    std::string oid_;
    std::unordered_map<std::string, std::string> client_hints_;
};


}  // namespace lite
}  // namespace catena
