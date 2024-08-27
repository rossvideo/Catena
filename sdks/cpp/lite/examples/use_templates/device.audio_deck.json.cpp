// This file was auto-generated. Do not modify by hand.
#include "device.audio_deck.json.h"
using namespace audio_deck;
#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <LanguagePack.h>
#include <Device.h>
#include <RangeConstraint.h>
#include <PicklistConstraint.h>
#include <NamedChoiceConstraint.h>
#include <Enums.h>
#include <StructInfo.h>
#include <string>
#include <vector>
#include <functional>
using catena::Device_DetailLevel;
using DetailLevel = catena::common::DetailLevel;
using catena::common::Scopes_e;
using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;
using catena::lite::StructInfo;
using catena::lite::FieldInfo;
using catena::lite::ParamDescriptor;
using catena::lite::ParamWithValue;
using catena::lite::Device;
using catena::lite::RangeConstraint;
using catena::lite::PicklistConstraint;
using catena::lite::NamedChoiceConstraint;
using catena::common::IParam;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::lite::Device dm {1, DetailLevel("FULL")(), {Scope("monitor")(), Scope("operate")(), Scope("configure")(), Scope("administer")()}, Scope("operate")(), false, false};

using catena::lite::LanguagePack;
const StructInfo& audio_deck::Eq::getStructInfo() {
  static StructInfo si {
    "Eq",
    {
      { "response", offsetof(Eq, response)},
      { "frequency", offsetof(Eq, frequency)},
      { "gain", offsetof(Eq, gain)},
      { "q_factor", offsetof(Eq, q_factor)}
    }
  };
  return si;
}
const StructInfo& audio_deck::Audio_channel::getStructInfo() {
  static StructInfo si {
    "Audio_channel",
    {
      { "fader", offsetof(Audio_channel, fader)},
      { "eq_list", offsetof(Audio_channel, eq_list)}
    }
  };
  return si;
}
audio_deck::AudioDeck audioDeck = {{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}}};
catena::lite::ParamWithValue<AudioDeck> _audio_deckParam {catena::ParamType::STRUCT_ARRAY, {}, {{"en", "Audio Deck"}}, {}, false, "audio_deck", dm, audioDeck};
