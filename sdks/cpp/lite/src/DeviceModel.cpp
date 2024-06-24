#include <lite/include/DeviceModel.h>
#include <lite/include/IParam.h>

using namespace catena::lite;

void DeviceModel::AddParam(const std::string& name, IParam* param) {
    params_[name] = param;
}

IParam* DeviceModel::GetParam(const std::string& name) {
    return params_[name];
}