#pragma once

#include <lite/include/IParam.h>
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>

#include <lite/param.pb.h>

#include <functional> // reference_wrapper

namespace catena {
namespace lite {

/**
 * @brief Param provides convenient access to parameter values
 * @tparam T the parameter's value type
 */
template <typename T> class Param : public IParam {
  public:
    /**
     * @brief Param does not have a default constructor
     */
    Param() = delete;

    /**
     * @brief Param does not have copy semantics
     */
    Param(T& value) = delete;

    /**
     * @brief Param does not have copy semantics
     */
    T& operator=(const T& value) = delete;

    /**
     * @brief Param has move semantics
     */
    Param(Param&& value) = default;

    /**
     * @brief Param has move semantics
     */
    Param& operator=(Param&& value) = default;

    /**
     * @brief default destructor
     */
    virtual ~Param() = default;

    /**
     * @brief the main constructor
     */
    Param(catena::ParamType type, T& value, const std::string& name, Device& dm) : IParam(), type_{type}, value_{value}, dm_{dm} {
        dm.addItem<Device::ParamTag>(name, this, Device::ParamTag{});
    }

    /**
     * @brief get the value of the parameter
     */
    inline T& get() { return value_.get(); }

    /**
     * @brief serialize the parameter value to protobuf
     */
    void toProto(catena::Value& value) const override;

    /**
     * @brief serialize the parameter descriptor to protobuf
     */
    void toProto(catena::Param& param) const override {
        param.set_type(type_);
        toProto(*param.mutable_value());
    }
    
  private:
    ParamType type_; // ParamType is from param.pb.h
    std::reference_wrapper<T> value_;
    std::reference_wrapper<Device> dm_;
};

}  // namespace lite
}  // namespace catena
