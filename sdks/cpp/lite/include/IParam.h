#pragma once

namespace catena {
class Value; // forward reference
namespace lite {
class IParam {
  public:
    IParam() = default;
    IParam(IParam&&) = default;
    IParam& operator=(IParam&&) = default;
    virtual ~IParam() = default;

    /**
     * @brief serialize the parameter value to protobuf
     */
    virtual void serialize(catena::Value& value) const = 0;
};
}  // namespace lite
}  // namespace catena