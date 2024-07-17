#pragma once

namespace catena {
class Value; // forward reference
class Param; // forward reference

namespace lite {
class IParam {
  public:
    IParam() = default;
    IParam(IParam&&) = default;
    IParam& operator=(IParam&&) = default;
    virtual ~IParam() = default;

    /**
     * @brief serialize the parameter value to protobuf
     * @param value the protobuf value to serialize to
     */
    virtual void toProto(catena::Value& value) const = 0;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param param the protobuf value to serialize to
     */
    virtual void toProto(catena::Param& param) const = 0; 
};
}  // namespace lite
}  // namespace catena