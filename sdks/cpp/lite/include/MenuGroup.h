#pragma once

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
#include <interface/menu.pb.h>

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
     * @brief construct a Menu Group from a list of Menus
     * @param name the name of the Menu Group
     * @param menus The menus in the group
     */
    MenuGroup(const PolyglotText& name, const std::initializer_list<std::pair<std::string, catena::Menu>>& menus);

    /**
     * @brief deserialize a menu group from a protobuf message
     * @param menuGroup the protobuf message
     */
    void fromProto(const ::catena::MenuGroup& menuGroup) override;

    /**
     * @brief serialize a menu group to a protobuf message
     * @param menuGroup the protobuf message
     */
    void toProto(::catena::MenuGroup& menuGroup) const override;

    /**
     * @brief get the name of the menu group
     * @return the name of the menu group
     */
    const PolyglotText& getName() const;

    /**
     * @brief set the name of the menu group
     * @param name the name of the menu group
     */
    void setName(const PolyglotText& name);

    /**
     * @brief set the menus in the group
     * @param menus the menus in the group
     */
    void setMenus(const std::unordered_map<std::string, Menu>& menus);

    /**
     * @brief add a menu to the group
     * @param key the key of the menu
     * @param menu the menu
     */

    void addMenu(const std::string& key, const Menu& menu);

    /**
     * @brief remove a menu from the group
     * @param key the key of the menu
     */
    void removeMenu(const std::string& key);

    /**
     * @brief check if the group has a menu
     * @param key the key of the menu
     * @return true if the group has the menu
     */
    bool hasMenu(const std::string& key) const;

    /**
     * @brief get a menu from the group
     * @param key the key of the menu
     * @return the menu
     */
    const Menu& getMenu(const std::string& key) const;

    /**
     * @brief get the menus in the group
     * @return the menus in the group
     */
    const std::unordered_map<std::string, Menu>& getMenus() const;

  private:
    PolyglotText name_;
    std::unordered_map<std::string, catena::Menu> menus_;
  
};


}  // namespace lite
}  // namespace catena