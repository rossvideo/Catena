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
 * @file MenuGroup.h
 * @brief MenuGroups support
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IMenuGroup.h>
#include <PolyglotText.h>
#include <IDevice.h>

#include <initializer_list>

namespace catena {
namespace common {

/**
 * @brief A Group of device menus
 */
class MenuGroup : public IMenuGroup {

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
    ~MenuGroup() = default;

    /**
     * @brief construct a Menu Group from a list of Menus
     * @param oid the oid of the Menu Group
     * @param name the name of the Menu Group
     * @param dev the device to add the menu group to
     */
    MenuGroup(std::string oid, const PolyglotText::ListInitializer name, IDevice& dev) : name_{name} {
        dev.addItem(oid, this);
    }

    /**
     * @brief serialize a menu group to a protobuf message
     * @param menuGroup the protobuf message
     */
    void toProto(::catena::MenuGroup& menuGroup, bool shallow) const override;

    /**
     * @brief add a menu to the group using move semantics
     * @param oid the key of the menu
     * @param menu the menu
     */
    void addMenu(const std::string& oid, std::unique_ptr<IMenu> menu) override {
        if (menu) { // Sanitzing menu first.
            menus_.emplace(oid, std::move(menu));
        } else {
            throw std::runtime_error("Cannot add a nullptr menu to MenuGroup");
        }
    }

    /**
     * @brief get menus from menu group
     * @return a map of menus
     */
    const MenuMap* menus() const override { return &menus_; }

  private:
    PolyglotText name_;
    MenuMap menus_;
};


}  // namespace common
}  // namespace catena
