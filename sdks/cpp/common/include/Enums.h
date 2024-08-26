#pragma once

// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @brief tools for using EnumDecorators.
 * @file Enums.h
 * @author John R. Naylor
 * @copyright Copyright (c) 2022, Ross Video Limited. All Rights Reserved.
 */

// common
#include <patterns/EnumDecorator.h>

// protobuf interface
#include <interface/device.pb.h>

#include <cstdint>


namespace catena {
namespace common {

/**
 * @brief Scopes_e is an enumeration of the different access scopes that a device supports.
 */
enum class Scopes_e : int32_t { kUndefined, kMonitor, kOperate, kConfig, kAdmin };


// instantiate the EnumDecorator for the Scopes_e and DetailLevel_e enums
using Scopes = patterns::EnumDecorator<common::Scopes_e>;
using DetailLevel = patterns::EnumDecorator<catena::Device_DetailLevel>;

}  // namespace common


template <>
inline const common::Scopes::FwdMap common::Scopes::fwdMap_ = {{common::Scopes_e::kUndefined, "undefined"},
                                               {common::Scopes_e::kMonitor, "monitor"},
                                               {common::Scopes_e::kOperate, "operate"},
                                               {common::Scopes_e::kConfig, "configure"},
                                               {common::Scopes_e::kAdmin, "administer"}};


template <>
inline const common::DetailLevel::FwdMap common::DetailLevel::fwdMap_ = {
  {Device_DetailLevel_FULL, "FULL"},
  {Device_DetailLevel_SUBSCRIPTIONS, "SUBSCRIPTIONS"},
  {Device_DetailLevel_MINIMAL, "MINIMAL"},
  {Device_DetailLevel_COMMANDS, "COMMANDS"},
  {Device_DetailLevel_NONE, "NONE"}};


}  // namespace catena
