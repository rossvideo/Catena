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
 * @file Menu.h
 * @brief Menus support
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

//common
#include <IMenu.h>

// lite
#include <Device.h>

// protobuf interface
#include <interface/menu.pb.h> 

#include <string>
#include <unordered_map>
#include <vector>

namespace catena {
namespace lite {

class Menu;  // forward declaration

/**
 * @brief Menu list
 */
class Menu : public common::IMenu {
  
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
     */
    Menu(const PolyglotText& name, bool hidden, bool disabled, \
    const std::initializer_list<std::string>& param_oids, \
    const std::initializer_list<std::string>& command_oids, \
    const std::initializer_list<std::pair<std::string, std::string>>& client_hints, \
    catena::lite::MenuGroup& menuGroup);

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

    /**
     * @brief get the hidden status of the menu
     * @return true if the menu is hidden
     */
    bool isHidden() const;

    /** @brief get the disabled status of the menu
     * @return true if the menu is disabled
     */
    bool isDisabled() const;

    /**
     * @brief get the param_oids of the menu
     * @return the list of parameter oids
     */ 
    const std::vector<std::string>& getParamOids() const;

    /**
     * @brief get the command_oids of the menu
     * @return the list of command oids
     */
    const std::vector<std::string>& getCommandOids() const;

    /**
     * @brief get the client_hints of the menu
     * @return the map of client hints
     */
    const std::unordered_map<std::string, std::string>& getClientHints() const;

    /**
     * @brief get the name of the menu
     * @return the name of the menu
     */
    const PolyglotText& getName() const { return name_; }

    /**
     * @brief set the name of the menu
     * @param name the name of the menu
     */ 
    void setName(const PolyglotText& name) {}

    /**
     * @brief set the hidden status of the menu
     * @param hidden true if the menu is hidden
     */
    void setHidden(bool hidden) {}

    /**
     * @brief set the disabled status of the menu
     * @param disabled true if the menu is disabled
     */
    void setDisabled(bool disabled) {}

    /**
     * @brief set the param_oids of the menu
     * @param param_oids the list of parameter oids
     */
    void setParamOids(const std::vector<std::string>& param_oids) {}

    /**
     * @brief set the command_oids of the menu
     * @param command_oids the list of command oids
     */
    void setCommandOids(const std::vector<std::string>& command_oids) {}

    /**
     * @brief set the client_hints of the menu
     * @param client_hints the map of client hints
     */
    void setClientHints(const std::unordered_map<std::string, std::string>& client_hints) {}


    private:
    PolyglotText name_;
    bool hidden_;
    bool disabled_;
    std::vector<std::string> param_oids_;
    std::vector<std::string> command_oids_;
    std::unordered_map<std::string, std::string> client_hints_;
};


}  // namespace lite
}  // namespace catena
