// This file was auto-generated. Do not modify by hand.
#include "example.tiny.json.h"

tiny::DeviceModel::DeviceModel():
    paramInfoMap(paramInfoMapInit()),
    a_number(paramInfoMap.at("/a_number"), 42)
{}

std::map<std::string, ParamInfo> tiny::DeviceModel::paramInfoMapInit()
{
    std::map<std::string, ParamInfo> m;
    m.insert(std::make_pair("/a_number", ParamInfo("A Number", "operate", "", Constraint())));
    return m;
}