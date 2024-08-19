#pragma once

#include <lite/include/IParam.h>
#include <lite/include/ParamDescriptor.h>
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>
#include <lite/include/PolyglotText.h>
#include <lite/include/Tags.h>

#include <lite/param.pb.h>

#include <lite/param.pb.h>

#include <functional>
#include <string>

namespace catena {
namespace lite {

template <typename T>
class ParamWithValue : public catena::common::IParam {
  public:
    using Descriptor = ParamDescriptor<T>;
  public:
    ParamWithValue() = delete;
    ParamWithValue(
        catena::ParamType type,
        const OidAliases& oid_aliases,
        const PolyglotText::ListInitializer name,
        const std::string& widget,
        const bool read_only,
        const std::string& oid,
        Device &dev,
        T& value
    ) : descriptor_{type, oid_aliases, name, widget, read_only, oid, dev}, 
        value_{value} {
        dm.addItem<common::ParamTag>(oid, this);
    }



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
        descriptor_.toProto(param);
    }

    typename IParam::ParamType type() const override {
        return descriptor_.type();
    }

    const std::string& getOid() const override {
        return descriptor_.getOid();
    }

    void setOid(const std::string& oid) override{
        descriptor_.setOid(oid);
    }

    bool readOnly() const override {
        return descriptor_.readOnly();
    }

    void readOnly(bool flag) override {
        descriptor_.readOnly(flag);
    }

    /**
     * @brief get the value of the parameter
     */
    T& get() {
        return value_.get();
    }

  private:
    Descriptor descriptor_;
    std::reference_wrapper<T> value_;
};

} // namespace lite
} // namespace catena
