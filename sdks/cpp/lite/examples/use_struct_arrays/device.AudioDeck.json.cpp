// This file was auto-generated. Do not modify by hand.
#include "device.AudioDeck.json.h"
using namespace AudioDeck;
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
template<>
struct catena::lite::StructInfo<AudioDeck::Eq> {
  using Eq = AudioDeck::Eq;
  using Type = std::tuple<FieldInfo<int32_t, Eq>, FieldInfo<float, Eq>, FieldInfo<float, Eq>, FieldInfo<float, Eq>>;
  static constexpr Type fields = {{"response", &Eq::response}, {"frequency", &Eq::frequency}, {"gain", &Eq::gain}, {"q_factor", &Eq::q_factor}};
};
template<>
struct catena::lite::StructInfo<AudioDeck::Audio_channel> {
  using Audio_channel = AudioDeck::Audio_channel;
  using Type = std::tuple<FieldInfo<float, Audio_channel>, FieldInfo<Audio_channel::Eq_list, Audio_channel>>;
  static constexpr Type fields = {{"fader", &Audio_channel::fader}, {"eq_list", &Audio_channel::eq_list}};
};
AudioDeck::Audio_deck audio_deck {{0.1,{{1,100,0,0},{2,200,0,0}}},{0.2,{{3,300,0,0},{4,400,0,0}}},{0.3,{{5,500,0,0},{6,600,0,0}}},{0.4,{{7,700,0,0},{8,800,0,0}}}};
catena::lite::ParamDescriptor _audio_deckDescriptor {catena::ParamType::STRUCT_ARRAY, {}, {{"en", "Audio Deck"}}, {}, "", false, "audio_deck", nullptr, dm};
catena::lite::ParamDescriptor _audio_deck_faderDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Fader"}}, {}, "", false, "fader", &_audio_deckDescriptor, dm};
catena::lite::ParamDescriptor _audio_deck_eq_listDescriptor {catena::ParamType::STRUCT_ARRAY, {}, {{"en", "EQ List"}}, {}, "", false, "eq_list", &_audio_deckDescriptor, dm};
catena::lite::ParamDescriptor _audio_deck_eq_list_responseDescriptor {catena::ParamType::INT32, {}, {{"en", "Response"}}, {}, "", false, "response", &_audio_deck_eq_listDescriptor, dm};
catena::lite::ParamDescriptor _audio_deck_eq_list_frequencyDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Frequency"}}, {}, "", false, "frequency", &_audio_deck_eq_listDescriptor, dm};
catena::lite::ParamDescriptor _audio_deck_eq_list_gainDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Gain"}}, {}, "", false, "gain", &_audio_deck_eq_listDescriptor, dm};
catena::lite::ParamDescriptor _audio_deck_eq_list_q_factorDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Q Factor"}}, {}, "", false, "q_factor", &_audio_deck_eq_listDescriptor, dm};
catena::lite::ParamWithValue<AudioDeck::Audio_deck> _audio_deckParam {audio_deck, _audio_deckDescriptor, dm};


// template<>
// struct catena::lite::StructInfo<AudioDeck::Eq> {
//   using Eq = AudioDeck::Eq;
//   using Type = std::tuple<FieldInfo<int32_t, Eq>, FieldInfo<float, Eq>, FieldInfo<float, Eq>, FieldInfo<float, Eq>>;
//   static constexpr Type fields = {{"response", &Eq::response}, {"frequency", &Eq::frequency}, {"gain", &Eq::gain}, {"q_factor", &Eq::q_factor}};
// };
// // template<>
// // struct catena::lite::StructInfo<AudioDeck::Audio_channel> {
// //   using Audio_channel = AudioDeck::Audio_channel;
// //   using Type = std::tuple<FieldInfo<float, Audio_channel>, FieldInfo<Audio_channel::Eq_list, Audio_channel>>;
// //   static constexpr Type fields = {{"fader", &Audio_channel::fader}, {"eq_list", &Audio_channel::eq_list}};
// // };
// AudioDeck::Audio_deck audio_deck {{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}},{0.5,{{1,100,0,0},{2,200,0,0}}}};
// catena::lite::ParamDescriptor _audio_deckDescriptor {catena::ParamType::STRUCT_ARRAY, {}, {{"en", "Audio Deck"}}, {}, "", false, "audio_deck", nullptr, dm};
// catena::lite::ParamDescriptor _audio_deck_faderDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Fader"}}, {}, "", false, "fader", &_audio_deckDescriptor, dm};
// catena::lite::ParamDescriptor _audio_deck_eq_listDescriptor {catena::ParamType::STRUCT_ARRAY, {}, {{"en", "EQ List"}}, {}, "", false, "eq_list", &_audio_deckDescriptor, dm};
// catena::lite::ParamDescriptor _audio_deck_eq_list_responseDescriptor {catena::ParamType::INT32, {}, {{"en", "Response"}}, {}, "", false, "response", &_audio_deck_eq_listDescriptor, dm};
// catena::lite::ParamDescriptor _audio_deck_eq_list_frequencyDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Frequency"}}, {}, "", false, "frequency", &_audio_deck_eq_listDescriptor, dm};
// catena::lite::ParamDescriptor _audio_deck_eq_list_gainDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Gain"}}, {}, "", false, "gain", &_audio_deck_eq_listDescriptor, dm};
// catena::lite::ParamDescriptor _audio_deck_eq_list_q_factorDescriptor {catena::ParamType::FLOAT32, {}, {{"en", "Q Factor"}}, {}, "", false, "q_factor", &_audio_deck_eq_listDescriptor, dm};
// catena::lite::ParamWithValue<AudioDeck::Audio_deck> _audio_deckParam {audio_deck, _audio_deckDescriptor, dm};
