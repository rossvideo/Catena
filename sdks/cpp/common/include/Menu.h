#pragma once

/*
 * Copyright 2024 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Menu.h
 * @brief Menus support
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IMenu.h>
#include <IMenuGroup.h>
#include <IDevice.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/menu.pb.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace catena {
namespace common {

/**
 * @brief A device menu
 */
class Menu : public IMenu {
  public:
    /**
     * @brief a list of oids, used by main constructor
     */
    using OidInitializer = std::initializer_list<std::string>;

    /**
     * @brief a list of string pairs, used by main constructor
     */
    using PairInitializer = std::initializer_list<std::pair<std::string, std::string>>;

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
    ~Menu() = default;

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
    Menu(const catena::common::PolyglotText::ListInitializer name, bool hidden, bool disabled,
         const OidInitializer param_oids,
         const OidInitializer command_oids,
         const PairInitializer& client_hints, 
         std::string oid, IMenuGroup& menuGroup);


    /**
     * @brief serialize a menu to a protobuf message
     * @param menu the protobuf message
     */
    void toProto(::st2138::Menu& menu) const override;

  private:
    /**
     * @brief The menu's name.
     */
    PolyglotText name_;
    /**
     * @brief Flag indicating if the menu is hidden.
     */
    bool hidden_;
    /**
     * @brief Flag indicating if the menu is disabled.
     */
    bool disabled_;
    /**
     * @brief A list of the param oids contained in this menu.
     */
    std::vector<std::string> param_oids_;
    /**
     * @brief A list of the command oids contained in this menu.
     */
    std::vector<std::string> command_oids_;
    /**
     * @brief A collection of client hints for use with this menu.
     */
    std::unordered_map<std::string, std::string> client_hints_;
};


}  // namespace common
}  // namespace catena
