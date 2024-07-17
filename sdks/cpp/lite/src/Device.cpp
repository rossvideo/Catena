#include <lite/include/Device.h>
#include <lite/include/IParam.h>

#include <cassert>

using namespace catena::lite;
using namespace catena::common;


// #define ADD_ITEM(MAP, TAG) \
// template<> \
// void Device::addItem<Device::TAG>(const std::string& name, Device::TAG::type* item) { \
//     assert(item != nullptr); \
//     MAP[name] = item; \
// }

// ADD_ITEM(params_, ParamTag)
// ADD_ITEM(commands_, CommandTag)
// ADD_ITEM(constraints_, ConstraintTag)
// ADD_ITEM(menu_groups_, MenuGroupTag)
// ADD_ITEM(language_packs_, LanguagePackTag)



// #define GET_ITEM(MAP, TAG) \
// template<> \
// Device::TAG::type* Device::getItem<Device::TAG>(const std::string& name, TAG tag) const { \
//     auto it = MAP.find(name); \
//     if (it != MAP.end()) { \
//         return it->second; \
//     } \
//     return nullptr; \
// }

// GET_ITEM(params_, ParamTag)
// GET_ITEM(commands_, CommandTag)
// GET_ITEM(constraints_, ConstraintTag)
// GET_ITEM(menu_groups_, MenuGroupTag)
// GET_ITEM(language_packs_, LanguagePackTag)



