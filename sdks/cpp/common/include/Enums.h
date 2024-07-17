#pragma once

#include <patterns/EnumDecorator.h>

namespace catena {
namespace common {

/**
 * @brief Scopes_e is an enumeration of the different access scopes that a device supports.
 */
enum class Scopes_e : int32_t { kUndefined, kMonitor, kOperate, kConfig, kAdmin };

/**
 * @brief DetailLevel_e is an enumeration of the different levels of detail that can be provided.
 */
enum class DetailLevel_e : int32_t { kFull, kSubscriptions, kMinimal, kCommands, kNone };


}  // namespace common
template <>
inline const patterns::EnumDecorator<common::Scopes_e>::FwdMap
  patterns::EnumDecorator<common::Scopes_e>::fwdMap_ = {{common::Scopes_e::kUndefined, "undefined"},
                                                        {common::Scopes_e::kMonitor, "monitor"},
                                                        {common::Scopes_e::kOperate, "operate"},
                                                        {common::Scopes_e::kConfig, "configure"},
                                                        {common::Scopes_e::kAdmin, "administer"}};

template <>
inline const patterns::EnumDecorator<common::DetailLevel_e>::FwdMap
  patterns::EnumDecorator<common::DetailLevel_e>::fwdMap_ = {{common::DetailLevel_e::kFull, "FULL"},
                                                             {common::DetailLevel_e::kSubscriptions, "SUBSCRIPTIONS"},
                                                             {common::DetailLevel_e::kMinimal, "MINIMAL"},
                                                             {common::DetailLevel_e::kCommands, "COMMANDS"},
                                                             {common::DetailLevel_e::kNone, "NONE"}};
}  // namespace catena
