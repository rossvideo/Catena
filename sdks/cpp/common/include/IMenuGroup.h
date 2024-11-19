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
 * @file IMenuGroup.h
 * @brief Interface for MenuGroup
 * @author Ben Mostafa Ben.Mostafa@rossvideo.com
 * @date 2024-10-04
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <string>
#include <unordered_map>

namespace catena {

class MenuGroup; // forward reference

namespace common{
    class Menu; // forward reference
}

namespace common {

/**
 * @brief Interface for Menu Group
 */
class IMenuGroup {
public:
    IMenuGroup() = default;
    IMenuGroup(IMenuGroup&&) = default;
    IMenuGroup& operator=(IMenuGroup&&) = default;
    virtual ~IMenuGroup() = default;

    /**
     * @brief serialize a menu group to a protobuf message
     * @param menuGroup the protobuf message
     */
    virtual void toProto(catena::MenuGroup& menuGroup, bool shallow) const = 0;

    /**
     * @brief get menus from menu group
     * @return a map of menus
     */
    virtual const std::unordered_map<std::string, catena::common::Menu>* menus() const = 0;
};
}  // namespace common
}  // namespace catena
