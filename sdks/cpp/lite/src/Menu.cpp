/**THIS IS A COPY OF LanguagePack.cpp NOT READY FOR RELEASE */

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

// lite
#include <Menu.h>

using namespace catena::lite;
using catena::common::MenuGroupTag; // There is no MenuTag in the code

  Menu::Menu(const PolyglotText& name, bool hidden, bool disabled, const std::initializer_list<std::string>& param_oids, const std::initializer_list<std::string>& command_oids, const std::initializer_list<std::pair<std::string, std::string>>& client_hints) {
        name_ = name;
        hidden_ = hidden;
        disabled_ = disabled;
        for (const auto& oid : param_oids) {
            param_oids_.push_back(oid);
        }
        for (const auto& oid : command_oids) {
            command_oids_.push_back(oid);
        }
        for (const auto& [key, value] : client_hints) {
            client_hints_[key] = value;
        }
    }

    void Menu::fromProto(const ::catena::Menu& menu) {
        name_ = menu.name();
        hidden_ = menu.hidden();
        disabled_ = menu.disabled();
        for (const auto& oid : menu.param_oids()) {
            param_oids_.push_back(oid);
        }
        for (const auto& oid : menu.command_oids()) {
            command_oids_.push_back(oid);
        }
        for (const auto& [key, value] : menu.client_hints()) {
            client_hints_[key] = value;
        }
    }

    void Menu::toProto(::catena::Menu& menu) const {
        menu.set_name(name_);
        menu.set_hidden(hidden_);
        menu.set_disabled(disabled_);
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
    
    const std::vector<std::string>& Menu::getCommandOids() const {
        return command_oids_;
    }

    const std::vector<std::string>& Menu::getParamOids() const {
        return param_oids_;
    }

    const std::unordered_map<std::string, std::string>& Menu::getClientHints() const {
        return client_hints_;
    }

    bool Menu::isDisabled() const {
        return disabled_;
    }

    bool Menu::isHidden() const {
        return hidden_;
    }

    void Menu::setDisabled(bool disabled) {
        disabled_ = disabled;
    }

    void Menu::setHidden(bool hidden) {
        hidden_ = hidden;
    }

    void Menu::setName(const std::string& name) {
        name_ = name;
    }

    const std::string& Menu::getName() const {
        return name_;
    }

    void Menu::setClientHints(const std::unordered_map<std::string, std::string>& hints) {
        client_hints_ = hints;
    }

    void Menu::setCommandOids(const std::vector<std::string>& oids) {
        command_oids_ = oids;
    }

    void Menu::setParamOids(const std::vector<std::string>& oids) {
        param_oids_ = oids;
    }





