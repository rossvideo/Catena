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
 * @file Menu.cpp
 * @brief Implements the Menu class
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-10
 * @copyright Copyright (c) 2024 Ross Video
 */

// lite
#include <Menu.h>
#include <MenuGroup.h>

using namespace catena::lite;
using catena::common::MenuTag;

Menu::Menu(const catena::lite::PolyglotText::ListInitializer name, bool hidden, bool disabled, const OidInitializer param_oids,
           const OidInitializer command_oids, const PairInitializer& client_hints, std::string oid,
           catena::lite::MenuGroup& menuGroup)
    : name_{name}, hidden_{hidden}, disabled_{disabled}, param_oids_{param_oids}, command_oids_{command_oids},
      client_hints_{client_hints.begin(), client_hints.end()} {
    menuGroup.addMenu(oid, std::move(*this));
}


void Menu::toProto(::catena::Menu& menu) const {
    name_.toProto(*menu.mutable_name());
    menu.set_hidden(hidden_);
    menu.set_disabled(disabled_);
    menu.clear_param_oids();
    menu.clear_command_oids();
    menu.clear_client_hints();
    for (const auto& oid : param_oids_) {
        menu.add_param_oids(oid);
    }
    for (const auto& oid : command_oids_) {
        menu.add_command_oids(oid);
    }
    for (const auto& [key, value] : client_hints_) {
        (*menu.mutable_client_hints())[key] = value;
    }
}
