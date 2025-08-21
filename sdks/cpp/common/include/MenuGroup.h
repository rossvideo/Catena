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
     * @brief Destructor
     */
    ~MenuGroup() = default;

    /**
     * @brief Construct a Menu Group from a list of Menus.
     * @param oid The oid of the Menu Group.
     * @param name The name of the Menu Group.
     * @param dev The device to add the menu group to.
     */
    MenuGroup(std::string oid, const PolyglotText::ListInitializer name, IDevice& dev) : name_{name} {
        dev.addItem(oid, this);
    }

    /**
     * @brief serialize a menu group to a protobuf message.
     * @param menuGroup The protobuf message.
     * @param shallow If true, only the top-level info is copied.
     */
    void toProto(::catena::MenuGroup& menuGroup, bool shallow) const override;

    /**
     * @brief Add a menu to the group using move semantics.
     * @param oid The key of the menu.
     * @param menu The menu to add.
     */
    void addMenu(const std::string& oid, std::unique_ptr<IMenu> menu) override {
        // Sanitizes input before adding.
        if (!menu) {
            throw std::runtime_error("Cannot add a nullptr menu to MenuGroup");
        } else if (oid.empty()) {
            throw std::runtime_error("Cannot assign a menu to an empty oid in MenuGroup");
        } else {
            menus_.emplace(oid, std::move(menu));
        }
    }

    /**
     * @brief Gets menus from the menu group.
     * @return A map of the menu group's menus.
     */
    const MenuMap* menus() const override { return &menus_; }

  private:
    /**
     * @brief The menu group's name.
     */
    PolyglotText name_;
    /**
     * @brief A map of the menu group's menus.
     */
    MenuMap menus_;
};


}  // namespace common
}  // namespace catena
