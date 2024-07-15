#include <Enums.h>

using Scopes_e = catena::common::Scopes_e;
using Scopes = typename catena::patterns::EnumDecorator<Scopes_e>;

template<>
const Scopes::FwdMap Scopes::fwdMap_ = {
    {Scopes_e::kUndefined, "undefined"},
    {Scopes_e::kMonitor, "monitor"},
    {Scopes_e::kOperate, "operate"},
    {Scopes_e::kConfig, "configure"},
    {Scopes_e::kAdmin, "administer"}
};


using DetailLevel_e = catena::common::DetailLevel_e;
using DetailLevel = typename catena::patterns::EnumDecorator<DetailLevel_e>;
template<>
const DetailLevel::FwdMap DetailLevel::fwdMap_ = {
    {DetailLevel_e::kFull, "FULL"},
    {DetailLevel_e::kSubscriptions, "SUBSCRIPTIONS"},
    {DetailLevel_e::kMinimal, "MINIMAL"},
    {DetailLevel_e::kCommands, "COMMANDS"},
    {DetailLevel_e::kNone, "NONE"}
};

