#pragma once

namespace catena {
namespace lite {
class IParam {
  public:
    IParam() = default;
    IParam(IParam&&) = default;
    IParam& operator=(IParam&&) = default;
    virtual ~IParam() = default;
};
}  // namespace lite
}  // namespace catena