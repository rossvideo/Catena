// This file was auto-generated. Do not modify by hand.
#include "device.use_menus.json.h"
using namespace use_menus;
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
#include <Menu.h>
#include <MenuGroup.h>
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
LanguagePack es {
  "Spanish",
  {
    { "greeting", "Hola" },
    { "parting", "Adiós" }
  },
  dm
};
LanguagePack en {
  "English",
  {
    { "greeting", "Hello" },
    { "parting", "Goodbye" }
  },
  dm
};
LanguagePack fr {
  "French",
  {
    { "greeting", "Bonjour" },
    { "parting", "Adieu" }
  },
  dm
};

catena::lite::MenuGroup _statusGroup {"status", {{"en", "Status"}, {"es", "Estado"}, {"fr", "Statut"}}, dm};
catena::lite::Menu _statusMenu {{{"en", "Status"}, {"es", "Estado"}, {"fr", "Statut"}}, false, false, {"/counter", "/hello"}, {}, {}, "Status", _statusGroup};
catena::lite::MenuGroup _configGroup {"config", {{"en", "Config"}, {"es", "Configuración"}, {"fr", "Configuration"}}, dm};
catena::lite::Menu _configMenu {{{"en", "Config"}, {"es", "Configuración"}, {"fr", "Configuration"}}, false, false, {"/offset", "/button"}, {}, {}, "Config", _configGroup};
int32_t counter {0};
ParamDescriptor _counterDescriptor {catena::ParamType::INT32, {}, {{"en", "Counter"},{"es", "Contador"},{"fr", "Compteur"}}, {}, "", false, "counter", nullptr, nullptr, dm, false};
ParamWithValue<int32_t> _counterParam {counter, _counterDescriptor, dm, false};
std::string hello {"Hello, World!"};
ParamDescriptor _helloDescriptor {catena::ParamType::STRING, {}, {{"en", "Text Box"},{"es", "Caja de texto"},{"fr", "Boîte de texte"}}, {}, "configure", false, "hello", nullptr, nullptr, dm, false};
ParamWithValue<std::string> _helloParam {hello, _helloDescriptor, dm, false};
int32_t button {0};
catena::lite::NamedChoiceConstraint<int32_t> _buttonParamConstraint {{{0,{{"en","Off"},{"es","Apagado"},{"fr","Éteint"}}},{1,{{"en","On"},{"es","Encendido"},{"fr","Allumé"}}}}, false, "button", false};
ParamDescriptor _buttonDescriptor {catena::ParamType::INT32, {}, {{"en", "Button"},{"es", "Botón"},{"fr", "Bouton"}}, "button", "", false, "button", &_buttonParamConstraint, nullptr, dm, false};
catena::lite::ParamWithValue<int32_t> _buttonParam {button, _buttonDescriptor, dm, false};
int32_t offset {0};
RangeConstraint<int32_t> _offsetParamConstraint {-10, 10, 1, -10, 10, "offset", false};
ParamDescriptor _offsetDescriptor {catena::ParamType::INT32, {}, {{"en", "offset"}}, {}, "configure", false, "offset", &_offsetParamConstraint, nullptr, dm, false};
ParamWithValue<int32_t> _offsetParam {offset, _offsetDescriptor, dm, false};
// std::string dashboard_UI {"eo://status_update.grid"};
// ParamDescriptor _dashboard_UIDescriptor {catena::ParamType::STRING, {{"0xFF0E"}}, {}, {}, "", true, "dashboard_UI", nullptr, dm};
// ParamWithValue<std::string> _dashboard_UIParam {dashboard_UI, _dashboard_UIDescriptor, dm};
