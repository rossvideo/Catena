#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace audio_deck {
struct Eq {
  int32_t response;
  float frequency;
  float gain;
  float q_factor;
  static const catena::lite::StructInfo& getStructInfo();
};
using Eq_list = std::vector<Eq>;
struct Audio_channel {
  float fader;
  Eq_list eq_list;
  static const catena::lite::StructInfo& getStructInfo();
};
using AudioDeck = std::vector<Audio_channel>;
} // namespace audio_deck
