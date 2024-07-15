#pragma once

#include <patterns/EnumDecorator.h>

namespace catena {
    namespace common {

// clang-format push
// clang-format off
#define SCOPES ( \
    Scopes, \
    int32_t, \
    (kUndefined) "undefined", \
    (kMonitor) "monitor", \
    (kOperate) "operate", \
    (kConfig) "configure", \
    (kAdmin) "administer" \
)


#define DETAIL_LEVEL ( \
    DetailLevel, \
    int32_t, \
    (kFull) "FULL", \
    (kSubscriptions) "SUBSCRIPTIONS", \
    (kMinimal) "MINIMAL", \
    (kCommands) "COMMANDS", \
    (kNone) "NONE"  \
)
// clang-format pop

INSTANTIATE_ENUM(ENUMDECORATOR_DECLARATION, SCOPES);
INSTANTIATE_ENUM(ENUMDECORATOR_DECLARATION, DETAIL_LEVEL);

} // namespace common
} // namespace catena
