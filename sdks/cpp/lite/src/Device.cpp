#include <lite/include/Device.h>
#include <lite/include/IParam.h>

#include <cassert>

using namespace catena::lite;
using namespace catena::common;

void Device::AddParam(const std::string& name, IParam* param) {
    params_[name] = param;
}

IParam* Device::GetParam(Path& path)  {
    const Path::Segment& name = path.front();
    assert(std::holds_alternative<std::string>(name));
    
    IParam* param = params_[std::get<std::string>(name)];

    if (param == nullptr) {
        throw std::runtime_error("Device model parameter \"" + std::get<std::string>(name) + "\" does not exist");
    }
    return param;
}

IParam* Device::GetParam(const std::string& name)  {
    Path path(name);
    return GetParam(path);
}