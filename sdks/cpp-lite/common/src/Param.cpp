
#include "Param.h"

IntParam::IntParam(ParamInfo& p, int32_t value): 
    IParam(p), value_(value)
{
    // give corresponding paramInfo a pointer to this param
    paramInfo.param = this;    
};

std::unique_ptr<catena::Value> IntParam::getSerializedValue() const{
    std::unique_ptr<catena::Value> value = std::make_unique<catena::Value>();
    value->set_int32_value(value_);
    return value;
}