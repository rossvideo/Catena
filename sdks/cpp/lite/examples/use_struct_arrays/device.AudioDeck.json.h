#pragma once
// This file was auto-generated. Do not modify by hand.
#include <Device.h>
#include <StructInfo.h>
extern catena::lite::Device dm;
namespace AudioDeck {
struct Eq {
  int32_t response;
  float frequency;
  float gain;
  float q_factor;
  static void isCatenaStruct() {};
};
struct Audio_channel {
  float fader;
  using Eq_list = std::vector<AudioDeck::Eq>;
  Eq_list eq_list = {{0,0,0,0},{0,0,0,0}};
  static void isCatenaStruct() {};
};
using Audio_deck = std::vector<AudioDeck::Audio_channel>;
} // namespace AudioDeck
