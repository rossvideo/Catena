
#pragma once

#include <param.pb.h>

class IParam; // forward declaration

class Constraint{

};

class ParamInfo
{
  public:
    ParamInfo() = delete; 
    ParamInfo(const std::string& name, const std::string& scope, const std::string& templateOid, const Constraint& constraint):
        name(name), scope(scope), templateOid(templateOid), constraint(constraint){};

    const std::string name;

    const std::string scope;

    const std::string templateOid;

    const Constraint constraint;

    IParam* param;
};

class IParam
{
  public:
    IParam(ParamInfo& paramInfo): paramInfo(paramInfo){}
    virtual ~IParam() = default;

    //virtual catena::Param getSerializedParam() const;

    virtual std::unique_ptr<catena::Value> getSerializedValue() const = 0;

    //virtual void setSerializedValue(const catena::SetValuePayload& value);

  protected:
    ParamInfo& paramInfo;
};

class IntParam : public IParam
{
  public:
    IntParam(ParamInfo& paramInfo, int32_t value);

    //catena::Param getSerializedParam() const override;

    std::unique_ptr<catena::Value> getSerializedValue() const override;

    //void setSerializedValue(const catena::SetValuePayload& value) override;

  private:
    int32_t value_;
};
