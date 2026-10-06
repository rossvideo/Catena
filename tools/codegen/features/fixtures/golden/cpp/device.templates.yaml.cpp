// This file was auto-generated. Do not modify by hand.
#include "device.templates.yaml.h"
using namespace templates;
#include <ParamDescriptor.h>
#include <ParamWithValue.h>
#include <LanguagePack.h>
#include <Device.h>
#include <RangeConstraint.h>
#include <ChoiceConstraint.h>
#include <Enums.h>
#include <StructInfo.h>
#include <string>
#include <vector>
#include <functional>
#include <Menu.h>
#include <MenuGroup.h>
using st2138::Device_DetailLevel;
using DetailLevel = catena::common::DetailLevel;
using catena::common::Scopes_e;
using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;
using catena::common::FieldInfo;
using catena::common::ParamDescriptor;
using catena::common::ParamWithValue;
using catena::common::Device;
using catena::common::RangeConstraint;
using catena::common::ChoiceConstraint;
using catena::common::IParam;
using catena::common::EmptyValue;
using std::placeholders::_1;
using std::placeholders::_2;
using catena::common::ParamTag;
using ParamAdder = catena::common::AddItem<ParamTag>;
catena::common::Device dm {1, DetailLevel("FULL")(), {"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"}, "st2138:op", true, false};

using catena::common::LanguagePack;
using catena::common::Menu;
using catena::common::MenuGroup;
Product product{.name{"Cucumber Fixture"},.vendor{"Ross Video"},.version{"1.0.0"},.serial_number{"SN-7K9M-2024-XR485-BLU"}};
catena::common::ParamDescriptor _productDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:mon", true, false, "product", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _product_nameDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "name", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_vendorDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "vendor", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdkDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_st2138_sdk_versionDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "st2138_sdk_version", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamDescriptor _product_serial_numberDescriptor(st2138::ParamType::STRING, {}, {}, "", "", false, false, "serial_number", "", nullptr, false, false, dm, 0, 0, 0, false, &_productDescriptor);
catena::common::ParamWithValue<templates::Product> _productParam(product, _productDescriptor, dm, false);
catena::common::ParamDescriptor _channel_templateDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Channel Strip"}}, "", "", false, false, "channel_template", "", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _channel_template_labelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Label"}}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_templateDescriptor);
catena::common::RangeConstraint<float> _channel_template_gainConstraint(-96, 12, 0.5, -96, 12, "gain", false);
catena::common::ParamDescriptor _channel_template_gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Gain (dB)"}}, "fader", "", false, false, "gain", "", &_channel_template_gainConstraint, false, false, dm, 0, 0, 1, false, &_channel_templateDescriptor);
catena::common::ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE> _channel_template_muteConstraint({{0,{{"en","Unmuted"}}},{1,{{"en","Muted"}}}}, false, "mute", false);
catena::common::ParamDescriptor _channel_template_muteDescriptor(st2138::ParamType::INT32, {}, {{"en", "Mute"}}, "toggle", "", false, false, "mute", "", &_channel_template_muteConstraint, false, false, dm, 0, 0, 0, false, &_channel_templateDescriptor);
catena::common::ParamDescriptor _channel_template_eqDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "EQ"}}, "", "", false, false, "eq", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_templateDescriptor);
catena::common::ParamDescriptor _channel_template_eq_lowDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Low"}}, "", "", false, false, "low", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_template_eqDescriptor);
catena::common::ParamDescriptor _channel_template_eq_midDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Mid"}}, "", "", false, false, "mid", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_template_eqDescriptor);
catena::common::ParamDescriptor _channel_template_eq_highDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "High"}}, "", "", false, false, "high", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_template_eqDescriptor);
catena::common::ParamWithValue<catena::common::EmptyValue> _channel_templateParam(catena::common::emptyValue, _channel_templateDescriptor, dm, false);
Channel_template channel_left{.label{"Left"},.gain{0},.mute{0},.eq{.low{1.5},.mid{0},.high{-2}}};
catena::common::ParamDescriptor _channel_leftDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Left"}}, "", "", false, false, "channel_left", "channel_template", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _channel_left_labelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Label"}}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_leftDescriptor);
catena::common::ParamDescriptor _channel_left_gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Gain (dB)"}}, "fader", "", false, false, "gain", "", &_channel_template_gainConstraint, false, false, dm, 0, 0, 1, false, &_channel_leftDescriptor);
catena::common::ParamDescriptor _channel_left_muteDescriptor(st2138::ParamType::INT32, {}, {{"en", "Mute"}}, "toggle", "", false, false, "mute", "", &_channel_template_muteConstraint, false, false, dm, 0, 0, 0, false, &_channel_leftDescriptor);
catena::common::ParamDescriptor _channel_left_eqDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "EQ"}}, "", "", false, false, "eq", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_leftDescriptor);
catena::common::ParamDescriptor _channel_left_eq_lowDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Low"}}, "", "", false, false, "low", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_left_eqDescriptor);
catena::common::ParamDescriptor _channel_left_eq_midDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Mid"}}, "", "", false, false, "mid", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_left_eqDescriptor);
catena::common::ParamDescriptor _channel_left_eq_highDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "High"}}, "", "", false, false, "high", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_left_eqDescriptor);
catena::common::ParamWithValue<templates::Channel_template> _channel_leftParam(channel_left, _channel_leftDescriptor, dm, false);
Channel_template channel_right{.label{"Right"},.gain{-3.5},.mute{1},.eq{.low{1.5},.mid{0},.high{-2}}};
catena::common::ParamDescriptor _channel_rightDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Right"}}, "", "", false, false, "channel_right", "channel_template", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _channel_right_labelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Label"}}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_rightDescriptor);
catena::common::ParamDescriptor _channel_right_gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Gain (dB)"}}, "fader", "", false, false, "gain", "", &_channel_template_gainConstraint, false, false, dm, 0, 0, 1, false, &_channel_rightDescriptor);
catena::common::ParamDescriptor _channel_right_muteDescriptor(st2138::ParamType::INT32, {}, {{"en", "Mute"}}, "toggle", "", false, false, "mute", "", &_channel_template_muteConstraint, false, false, dm, 0, 0, 0, false, &_channel_rightDescriptor);
catena::common::ParamDescriptor _channel_right_eqDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "EQ"}}, "", "", false, false, "eq", "", nullptr, false, false, dm, 0, 0, 0, false, &_channel_rightDescriptor);
catena::common::ParamDescriptor _channel_right_eq_lowDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Low"}}, "", "", false, false, "low", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_right_eqDescriptor);
catena::common::ParamDescriptor _channel_right_eq_midDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Mid"}}, "", "", false, false, "mid", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_right_eqDescriptor);
catena::common::ParamDescriptor _channel_right_eq_highDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "High"}}, "", "", false, false, "high", "", nullptr, false, false, dm, 0, 0, 1, false, &_channel_right_eqDescriptor);
catena::common::ParamWithValue<templates::Channel_template> _channel_rightParam(channel_right, _channel_rightDescriptor, dm, false);
Aux_sends aux_sends{{.label{"Aux 1"},.gain{-6},.mute{0},.eq{.low{0},.mid{0},.high{0}}},{.label{"Aux 2"},.gain{-12},.mute{1},.eq{.low{0},.mid{0},.high{0}}}};
catena::common::ParamDescriptor _aux_sendsDescriptor(st2138::ParamType::STRUCT_ARRAY, {}, {{"en", "Aux Sends"}}, "", "", false, false, "aux_sends", "channel_template", nullptr, false, false, dm, 0, 0, 0, false, nullptr);
catena::common::ParamDescriptor _aux_sends_labelDescriptor(st2138::ParamType::STRING, {}, {{"en", "Label"}}, "", "", false, false, "label", "", nullptr, false, false, dm, 0, 0, 0, false, &_aux_sendsDescriptor);
catena::common::ParamDescriptor _aux_sends_gainDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Gain (dB)"}}, "fader", "", false, false, "gain", "", &_channel_template_gainConstraint, false, false, dm, 0, 0, 1, false, &_aux_sendsDescriptor);
catena::common::ParamDescriptor _aux_sends_muteDescriptor(st2138::ParamType::INT32, {}, {{"en", "Mute"}}, "toggle", "", false, false, "mute", "", &_channel_template_muteConstraint, false, false, dm, 0, 0, 0, false, &_aux_sendsDescriptor);
catena::common::ParamDescriptor _aux_sends_eqDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "EQ"}}, "", "", false, false, "eq", "", nullptr, false, false, dm, 0, 0, 0, false, &_aux_sendsDescriptor);
catena::common::ParamDescriptor _aux_sends_eq_lowDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Low"}}, "", "", false, false, "low", "", nullptr, false, false, dm, 0, 0, 1, false, &_aux_sends_eqDescriptor);
catena::common::ParamDescriptor _aux_sends_eq_midDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Mid"}}, "", "", false, false, "mid", "", nullptr, false, false, dm, 0, 0, 1, false, &_aux_sends_eqDescriptor);
catena::common::ParamDescriptor _aux_sends_eq_highDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "High"}}, "", "", false, false, "high", "", nullptr, false, false, dm, 0, 0, 1, false, &_aux_sends_eqDescriptor);
catena::common::ParamWithValue<templates::Aux_sends> _aux_sendsParam(aux_sends, _aux_sendsDescriptor, dm, false);
constexpr const char* real_sdk_version = CATENA_CPP_VERSION;
constexpr const char* real_sdk_url = CATENA_CPP_SDK;
templates::Product& initialize_sdk_version(templates::Product& p) {
  p.st2138_sdk = real_sdk_url;
  p.st2138_sdk_version = real_sdk_version;
  return p;
}
templates::Product dummy = initialize_sdk_version(product);
