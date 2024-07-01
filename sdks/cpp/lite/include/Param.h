#pragma once

#include <lite/include/IParam.h>
#include <lite/include/DeviceModel.h>
#include <lite/include/StructInfo.h>

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
    Param(T& value, const std::string& name, DeviceModel& dm) : IParam(), value_(value), dm_(dm) {
        dm.AddParam(name, this);
    }

    /**
     * @brief get the value of the parameter
     */
    T& Get() { return value_.get(); }

    /**
     * @brief serialize the parameter value to protobuf
     */
    void serialize(catena::Value& value) const override;

  private:
    void serializeStruct(Value& value, const StructInfo& si) const {
        std::cout << "StructInfo: " << si.name << std::endl;
    }
  private:
    std::reference_wrapper<T> value_;
    std::reference_wrapper<DeviceModel> dm_;
};
}  // namespace lite
}  // namespace catena