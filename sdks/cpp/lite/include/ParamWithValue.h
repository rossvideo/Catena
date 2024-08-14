#pragma once

#include <lite/include/IParam.h> // for now, move to common later
#include <lite/include/ParamDescriptor.h>

#include <lite/param.pb.h>

#include <functional>
#include <string>

namespace catena {
namespace lite {

template <typename T>
class ParamWithValue : public IParam {
  public:
    using Descriptor = ParamDescriptor<T>;
  public:
    ParamWithValue() = delete;
    ParamWithValue(Descriptor& desc, T& value) : descriptor_(desc), value_(value) {}
    ParamWithValue(const ParamWithValue&) = delete;
    ParamWithValue& operator=(const ParamWithValue&) = delete;
    ParamWithValue(ParamWithValue&&) = default;
    ParamWithValue& operator=(ParamWithValue&&) = default;
    virtual ~ParamWithValue() = default;

    void toProto(catena::Value& value) const override {
        
    }

    void fromProto(const catena::Value& value) override {
        
    }

    void toProto(catena::Param& param) const override {
        descriptor_.get().toProto(param);
    }

    typename IParam::ParamType type() const override {
        descriptor_.get().type();
    }

    const std::string& getOid() const override {
        descriptor_.get().getOid();
    }

    void setOid(const std::string& oid) {
        descriptor_.get().setOid(oid);
    }

    bool readOnly() const override {
        descriptor_.get().readOnly();
    }

    void readOnly(bool flag) override {
        descriptor_.get().readOnly(flag);
    }

    /**
     * @brief get the value of the parameter
     */
    T& get() const {
        return value_.get();
    }

  private:
    std::reference_wrapper<Descriptor> descriptor_;
    std::reference_wrapper<T> value_;
};

} // namespace lite
} // namespace catena
