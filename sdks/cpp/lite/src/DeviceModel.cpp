#include <lite/include/DeviceModel.h>
#include <lite/include/IParam.h>

#include <cassert>

using namespace catena::lite;
using namespace catena::common;

void DeviceModel::AddParam(const std::string& name, IParam* param) {
    params_[name] = param;
}

IParam* DeviceModel::GetParam(Path& path)  {
    const Path::Segment& name = path.front();
    assert(std::holds_alternative<std::string>(name));
    
    return params_[std::get<std::string>(name)];
}