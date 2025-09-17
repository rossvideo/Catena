// This file was auto-generated. Do not modify by hand.
#include "device.one_of_everything.yaml.h"
using namespace one_of_everything;
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
catena::common::Device dm {0, DetailLevel("FULL")(), {"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"}, "st2138:mon", true, false};

using catena::common::LanguagePack;
using catena::common::Menu;
using catena::common::MenuGroup;
MenuGroup _statusGroup {
  "status", 
  {
    { "en", "Status" },
    { "fr", "Statut" },
    { "es", "Estado" },
    { "ja", "ステータス" }
  },
  dm
};
Menu _statusGroup_productMenu {
  {    
    { "en", "Product" },
    { "fr", "Produit" },
    { "es", "Producto" },
    { "ja", "製品" }
  },
  false, false, 
  {"/counter", "/number_example"}, 
  {}, {}, "product", _statusGroup
};
MenuGroup _configGroup {
  "config", 
  {
    { "en", "Config" },
    { "fr", "Configuration" },
    { "es", "Configuración" },
    { "ja", "設定" }
  },
  dm
};
Menu _configGroup_catenaMenu {
  {    
    { "en", "Catena" },
    { "fr", "Catena" },
    { "es", "Catena" },
    { "ja", "カテナ" }
  },
  false, false, 
  {"/float_example", "/string_example", "/number_array", "/float_array", "/string_array"}, 
  {}, {}, "catena", _configGroup
};
Menu _configGroup_struct_examplesMenu {
  {    
    { "en", "Struct Examples" },
    { "fr", "Exemples de Structure" },
    { "es", "Ejemplos de Estructura" },
    { "ja", "構造体の例" }
  },
  false, false, 
  {"/struct_example", "/struct_example/nested_struct", "/struct_array", "/struct_array/0/nested_struct", "/struct_array/1/nested_struct"}, 
  {}, {}, "struct_examples", _configGroup
};
Menu _configGroup_constraint_examplesMenu {
  {    
    { "en", "Constraint Examples" },
    { "fr", "Exemples de Contraintes" },
    { "es", "Ejemplos de Restricciones" },
    { "ja", "制約の例" }
  },
  false, false, 
  {"/constraint_examples/int32_range", "/constraint_examples/int_array_range", "/constraint_examples/int32_choice", "/constraint_examples/int_array_choice", "/constraint_examples/float_range", "/constraint_examples/float_array_range", "/constraint_examples/string_choice", "/constraint_examples/string_array_choice", "/constraint_examples/string_string_choice", "/constraint_examples/string_string_array_choice", "/constraint_examples/string_length", "/constraint_examples/string_array_length"}, 
  {}, {}, "constraint_examples", _configGroup
};
Menu _configGroup_command_examplesMenu {
  {    
    { "en", "Command Examples" },
    { "fr", "Exemples de Commandes" },
    { "es", "Ejemplos de Comandos" },
    { "ja", "コマンド例" }
  },
  false, false, 
  {}, 
  {}, {{ "oglml", "eo://commands.grid" }}, "command_examples", _configGroup
};
std::string dashboard_UI{"eo://onedisplay.grid"};
catena::common::ParamDescriptor _dashboard_UIDescriptor(st2138::ParamType::STRING, {{"0xFF0E"}}, {}, "", "", true, "dashboard_UI", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::string> _dashboard_UIParam(dashboard_UI, _dashboard_UIDescriptor, dm, false);
std::string dashboard_icon{"eo://catena_logo_16x16.png"};
catena::common::ParamDescriptor _dashboard_iconDescriptor(st2138::ParamType::STRING, {{"0xFF0C"}}, {}, "", "", true, "dashboard_icon", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::string> _dashboard_iconParam(dashboard_icon, _dashboard_iconDescriptor, dm, false);
int32_t number_example{0};
catena::common::ParamDescriptor _number_exampleDescriptor(st2138::ParamType::INT32, {}, {{"en", "Int Example"},{"fr", "Exemple Entier"},{"es", "Ejemplo Entero"},{"ja", "整数の例"}}, "", "", false, "number_example", "", nullptr, false, false, dm, 0, 0, 2, true, nullptr);
catena::common::ParamWithValue<int32_t> _number_exampleParam(number_example, _number_exampleDescriptor, dm, false);
int32_t counter{0};
catena::common::ParamDescriptor _counterDescriptor(st2138::ParamType::INT32, {}, {{"en", "Counter"},{"fr", "Compteur"},{"es", "Contador"},{"ja", "カウンター"}}, "", "", false, "counter", "", nullptr, false, false, dm, 0, 0, 2, true, nullptr);
catena::common::ParamWithValue<int32_t> _counterParam(counter, _counterDescriptor, dm, false);
float float_example{0};
catena::common::ParamDescriptor _float_exampleDescriptor(st2138::ParamType::FLOAT32, {}, {{"en", "Float Example"},{"fr", "Exemple Flottant"},{"es", "Ejemplo Flotante"},{"ja", "浮動小数点の例"}}, "default", "st2138:op", false, "float_example", "", nullptr, false, false, dm, 0, 0, 1, false, nullptr);
catena::common::ParamWithValue<float> _float_exampleParam(float_example, _float_exampleDescriptor, dm, false);
std::string string_example{"Hello World"};
catena::common::ParamDescriptor _string_exampleDescriptor(st2138::ParamType::STRING, {}, {{"en", "String Example"},{"fr", "Exemple Chaîne"},{"es", "Ejemplo de Cadena"},{"ja", "文字列の例"}}, "", "st2138:cfg", false, "string_example", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::string> _string_exampleParam(string_example, _string_exampleDescriptor, dm, false);
std::vector<int32_t> number_array{1, 2, 3, 4};
catena::common::ParamDescriptor _number_arrayDescriptor(st2138::ParamType::INT32_ARRAY, {}, {{"en", "Int Array Example"},{"fr", "Exemple Tableau Entiers"},{"es", "Ejemplo de Matriz de Enteros"},{"ja", "整数配列の例"}}, "", "", false, "number_array", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::vector<int32_t>> _number_arrayParam(number_array, _number_arrayDescriptor, dm, false);
std::vector<float> float_array{1.1, 2.2, 3.3, 4.4};
catena::common::ParamDescriptor _float_arrayDescriptor(st2138::ParamType::FLOAT32_ARRAY, {}, {{"en", "Float Array Example"},{"fr", "Exemple Tableau Flottants"},{"es", "Ejemplo de Matriz Flotante"},{"ja", "浮動小数点配列の例"}}, "", "", false, "float_array", "", nullptr, false, false, dm, 0, 0, 3, false, nullptr);
catena::common::ParamWithValue<std::vector<float>> _float_arrayParam(float_array, _float_arrayDescriptor, dm, false);
std::vector<std::string> string_array{"one", "two", "three", "four", "five"};
catena::common::ParamDescriptor _string_arrayDescriptor(st2138::ParamType::STRING_ARRAY, {}, {{"en", "String Array Example"},{"fr", "Exemple Tableau Chaînes"},{"es", "Ejemplo de Matriz de Cadenas"},{"ja", "文字列配列の例"}}, "", "", false, "string_array", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<std::vector<std::string>> _string_arrayParam(string_array, _string_arrayDescriptor, dm, false);
int32_t menu_button{0};
catena::common::ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE> _menu_buttonConstraint({{0,{{"en","Popup Menu"},{"fr","Menu Popup"},{"es","Menú Emergente"},{"ja","ポップアップメニュー"}}}}, false, "menu_button", false);
catena::common::ParamDescriptor _menu_buttonDescriptor(st2138::ParamType::INT32, {}, {{"en", "Menu Button"},{"fr", "Bouton de Menu"},{"es", "Botón de Menú"},{"ja", "メニューボタン"}}, "menu_popup", "", false, "menu_button", "", &_menu_buttonConstraint, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<int32_t> _menu_buttonParam(menu_button, _menu_buttonDescriptor, dm, false);
catena::common::ParamDescriptor _ref_structDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Struct Example"},{"fr", "Exemple de Structure"},{"es", "Ejemplo de Estructura"},{"ja", "構造体の例"}}, "", "", false, "ref_struct", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamDescriptor _ref_struct_nested_structDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Nested Struct"},{"fr", "Structure Imbriquée"},{"es", "Estructura Anidada"},{"ja", "ネストされた構造体"}}, "", "st2158:op", false, "nested_struct", "", nullptr, false, false, dm, 0, 0, 2, false, &_ref_structDescriptor);
catena::common::ParamDescriptor _ref_struct_nested_struct_num_1Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 1"},{"fr", "Num 1"},{"es", "Num 1"},{"ja", "数値 1"}}, "", "st2138:cfg", false, "num_1", "", nullptr, false, false, dm, 0, 0, 2, false, &_ref_struct_nested_structDescriptor);
catena::common::ParamDescriptor _ref_struct_nested_struct_num_2Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 2"},{"fr", "Num 2"},{"es", "Num 2"},{"ja", "数値 2"}}, "", "", false, "num_2", "", nullptr, false, false, dm, 0, 0, 2, false, &_ref_struct_nested_structDescriptor);
catena::common::ParamWithValue<catena::common::EmptyValue> _ref_structParam(catena::common::emptyValue, _ref_structDescriptor, dm, false);
Ref_struct struct_example{.nested_struct{.num_1{1},.num_2{2}}};
catena::common::ParamDescriptor _struct_exampleDescriptor(st2138::ParamType::STRUCT, {}, {}, "", "st2138:op", false, "struct_example", "/ref_struct", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamDescriptor _struct_example_nested_structDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Nested Struct"},{"fr", "Structure Imbriquée"},{"es", "Estructura Anidada"},{"ja", "ネストされた構造体"}}, "", "st2158:op", false, "nested_struct", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_exampleDescriptor);
catena::common::ParamDescriptor _struct_example_nested_struct_num_1Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 1"},{"fr", "Num 1"},{"es", "Num 1"},{"ja", "数値 1"}}, "", "st2138:cfg", false, "num_1", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_example_nested_structDescriptor);
catena::common::ParamDescriptor _struct_example_nested_struct_num_2Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 2"},{"fr", "Num 2"},{"es", "Num 2"},{"ja", "数値 2"}}, "", "", false, "num_2", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_example_nested_structDescriptor);
catena::common::ParamWithValue<one_of_everything::Ref_struct> _struct_exampleParam(struct_example, _struct_exampleDescriptor, dm, false);
Struct_array struct_array{{.nested_struct{.num_1{1},.num_2{2}}},{.nested_struct{.num_1{3},.num_2{4}}}};
catena::common::ParamDescriptor _struct_arrayDescriptor(st2138::ParamType::STRUCT_ARRAY, {}, {{"en", "Struct Array Example"},{"fr", "Exemple de Tableau de Structures"},{"es", "Ejemplo de Arreglo de Estructuras"},{"ja", "構造体配列の例"}}, "", "st2138:adm", false, "struct_array", "/ref_struct", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamDescriptor _struct_array_nested_structDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Nested Struct"},{"fr", "Structure Imbriquée"},{"es", "Estructura Anidada"},{"ja", "ネストされた構造体"}}, "", "st2158:op", false, "nested_struct", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_arrayDescriptor);
catena::common::ParamDescriptor _struct_array_nested_struct_num_1Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 1"},{"fr", "Num 1"},{"es", "Num 1"},{"ja", "数値 1"}}, "", "st2138:cfg", false, "num_1", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_array_nested_structDescriptor);
catena::common::ParamDescriptor _struct_array_nested_struct_num_2Descriptor(st2138::ParamType::INT32, {}, {{"en", "Num 2"},{"fr", "Num 2"},{"es", "Num 2"},{"ja", "数値 2"}}, "", "", false, "num_2", "", nullptr, false, false, dm, 0, 0, 2, false, &_struct_array_nested_structDescriptor);
catena::common::ParamWithValue<one_of_everything::Struct_array> _struct_arrayParam(struct_array, _struct_arrayDescriptor, dm, false);
Constraint_examples constraint_examples{.int32_range{6},.int32_choice{1},.float32_range{10.5},.string_choice{"a"},.string_string_choice{"<#FF0000>"},.string_length{"Hello worl"},.int_array_range{0, 2, 4, 6, 8, 10},.int_array_choice{0, 1, 0, 0, 1},.float_array_range{11.5, 22.5, 33.5, 44.5},.string_array_choice{"a", "b", "a"},.string_string_array_choice{"<#FF0000>", "<#00FF00>", "<#0000FF>"},.string_array_length{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"}};
catena::common::ParamDescriptor _constraint_examplesDescriptor(st2138::ParamType::STRUCT, {}, {{"en", "Constraint Examples"},{"fr", "Exemples de Contraintes"},{"es", "Ejemplos de Restricciones"},{"ja", "制約の例"}}, "", "", false, "constraint_examples", "", nullptr, false, false, dm, 0, 0, 2, false, nullptr);
catena::common::RangeConstraint<int32_t> _constraint_examples_int32_rangeConstraint(0, 10, 2, 2, 8, "int32_range", false);
catena::common::ParamDescriptor _constraint_examples_int32_rangeDescriptor(st2138::ParamType::INT32, {}, {}, "slider_horizontal", "", false, "int32_range", "", &_constraint_examples_int32_rangeConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE> _constraint_examples_int32_choiceConstraint({{0,{{"en","Zero"},{"fr","Zéro"},{"es","Cero"},{"ja","ゼロ"}}},{1,{{"en","One"},{"fr","Un"},{"es","Uno"},{"ja","一"}}},{2,{{"en","Two"},{"fr","Deux"},{"es","Dos"},{"ja","二"}}},{3,{{"en","Three"},{"fr","Trois"},{"es","Tres"},{"ja","三"}}}}, false, "int32_choice", false);
catena::common::ParamDescriptor _constraint_examples_int32_choiceDescriptor(st2138::ParamType::INT32, {}, {}, "radio-vertical", "", false, "int32_choice", "", &_constraint_examples_int32_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::RangeConstraint<float> _constraint_examples_float32_rangeConstraint(0, 100, 0.5, 10, 90, "float32_range", false);
catena::common::ParamDescriptor _constraint_examples_float32_rangeDescriptor(st2138::ParamType::FLOAT32, {}, {}, "slider_horizontal_no_label", "", false, "float32_range", "", &_constraint_examples_float32_rangeConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE> _constraint_examples_string_choiceConstraint({{"a",{}},{"b",{}}}, true, "string_choice", false);
catena::common::ParamDescriptor _constraint_examples_string_choiceDescriptor(st2138::ParamType::STRING, {}, {}, "combo_entry", "", false, "string_choice", "", &_constraint_examples_string_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE> _constraint_examples_string_string_choiceConstraint({{"<#FF0000>",{{"en","Red"},{"fr","Rouge"},{"es","Rojo"},{"ja","赤"}}},{"<#00FF00>",{{"en","Green"},{"fr","Vert"},{"es","Verde"},{"ja","緑"}}},{"<#0000FF>",{{"en","Blue"},{"fr","Bleu"},{"es","Azul"},{"ja","青"}}}}, true, "string_string_choice", false);
catena::common::ParamDescriptor _constraint_examples_string_string_choiceDescriptor(st2138::ParamType::STRING, {}, {}, "combo_entry", "", false, "string_string_choice", "", &_constraint_examples_string_string_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ParamDescriptor _constraint_examples_string_lengthDescriptor(st2138::ParamType::STRING, {}, {}, "text_entry", "", false, "string_length", "", nullptr, false, false, dm, 10, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::RangeConstraint<int32_t> _constraint_examples_int_array_rangeConstraint(0, 10, 2, 2, 8, "int_array_range", false);
catena::common::ParamDescriptor _constraint_examples_int_array_rangeDescriptor(st2138::ParamType::INT32_ARRAY, {}, {}, "slider_vertical", "", false, "int_array_range", "", &_constraint_examples_int_array_rangeConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE> _constraint_examples_int_array_choiceConstraint({{0,{{"en","Off"},{"fr","Arrêt"},{"es","Apagado"},{"ja","オフ"}}},{1,{{"en","On"},{"fr","Marche"},{"es","Encendido"},{"ja","オン"}}}}, false, "int_array_choice", false);
catena::common::ParamDescriptor _constraint_examples_int_array_choiceDescriptor(st2138::ParamType::INT32_ARRAY, {}, {}, "button", "", false, "int_array_choice", "", &_constraint_examples_int_array_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::RangeConstraint<float> _constraint_examples_float_array_rangeConstraint(0, 100, 0.5, 10, 90, "float_array_range", false);
catena::common::ParamDescriptor _constraint_examples_float_array_rangeDescriptor(st2138::ParamType::FLOAT32_ARRAY, {}, {}, "slider_vertical_no_label", "", false, "float_array_range", "", &_constraint_examples_float_array_rangeConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE> _constraint_examples_string_array_choiceConstraint({{"a",{}},{"b",{}}}, true, "string_array_choice", false);
catena::common::ParamDescriptor _constraint_examples_string_array_choiceDescriptor(st2138::ParamType::STRING_ARRAY, {}, {}, "combo_entry", "", false, "string_array_choice", "", &_constraint_examples_string_array_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE> _constraint_examples_string_string_array_choiceConstraint({{"<#FF0000>",{{"en","Red"},{"fr","Rouge"},{"es","Rojo"},{"ja","赤"}}},{"<#00FF00>",{{"en","Green"},{"fr","Vert"},{"es","Verde"},{"ja","緑"}}},{"<#0000FF>",{{"en","Blue"},{"fr","Bleu"},{"es","Azul"},{"ja","青"}}}}, true, "string_string_array_choice", false);
catena::common::ParamDescriptor _constraint_examples_string_string_array_choiceDescriptor(st2138::ParamType::STRING_ARRAY, {}, {}, "combo_entry", "", false, "string_string_array_choice", "", &_constraint_examples_string_string_array_choiceConstraint, false, false, dm, 0, 0, 2, false, &_constraint_examplesDescriptor);
catena::common::ParamDescriptor _constraint_examples_string_array_lengthDescriptor(st2138::ParamType::STRING_ARRAY, {}, {}, "text_entry", "", false, "string_array_length", "", nullptr, false, false, dm, 10, 100, 2, false, &_constraint_examplesDescriptor);
catena::common::ParamWithValue<one_of_everything::Constraint_examples> _constraint_examplesParam(constraint_examples, _constraint_examplesDescriptor, dm, false);
catena::common::ParamDescriptor _fib_startDescriptor(st2138::ParamType::EMPTY, {}, {}, "", "", false, "fib_start", "", nullptr, true, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<catena::common::EmptyValue> _fib_startParam(catena::common::emptyValue, _fib_startDescriptor, dm, true);
catena::common::ParamDescriptor _fib_stopDescriptor(st2138::ParamType::EMPTY, {}, {}, "", "", false, "fib_stop", "", nullptr, true, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<catena::common::EmptyValue> _fib_stopParam(catena::common::emptyValue, _fib_stopDescriptor, dm, true);
catena::common::ParamDescriptor _fib_setDescriptor(st2138::ParamType::INT32, {}, {}, "", "", false, "fib_set", "", nullptr, true, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<catena::common::EmptyValue> _fib_setParam(catena::common::emptyValue, _fib_setDescriptor, dm, true);
catena::common::ParamDescriptor _randomizeDescriptor(st2138::ParamType::EMPTY, {}, {}, "", "", false, "randomize", "", nullptr, true, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<catena::common::EmptyValue> _randomizeParam(catena::common::emptyValue, _randomizeDescriptor, dm, true);
catena::common::ParamDescriptor _tape_botDescriptor(st2138::ParamType::EMPTY, {}, {}, "", "", false, "tape_bot", "", nullptr, true, false, dm, 0, 0, 2, false, nullptr);
catena::common::ParamWithValue<catena::common::EmptyValue> _tape_botParam(catena::common::emptyValue, _tape_botDescriptor, dm, true);
