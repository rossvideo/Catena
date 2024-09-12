#pragma once

// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @file ParamWithValue.h
 * @brief Interface for acessing parameter values
 * @author John R. Naylor
 * @date 2024-08-20
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <IParam.h>
#include <Tags.h>

// lite
#include <ParamDescriptor.h>
#include <Device.h>
#include <StructInfo.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

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
        const std::string& scope,
        const bool read_only,
        const std::string& oid,
        Device &dev,
        T& value
    ) : descriptor_{type, oid_aliases, name, widget, scope, read_only, oid, dev}, 
        value_{value} {
        dm.addItem<common::ParamTag>(oid, this);
    }



    ParamWithValue(const ParamWithValue&) = delete;
    ParamWithValue& operator=(const ParamWithValue&) = delete;
    ParamWithValue(ParamWithValue&&) = default;
    ParamWithValue& operator=(ParamWithValue&&) = default;
    virtual ~ParamWithValue() = default;

    void toProto(catena::Value& value) const override {
        catena::lite::toProto<T>(value, &value_.get());
    }

    void fromProto(catena::Value& value) override {
        
    }

    void toProto(catena::Param& param) const override {
        descriptor_.toProto(param);        
        catena::lite::toProto<T>(*param.mutable_value(), &value_.get());
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

    /**
     * @brief get a child parameter by name
     */
    IParam* getParam(const std::string& oid) override {
        return descriptor_.getParam(oid);
    }

    /**
     * @brief add a child parameter
     */
    void addParam(const std::string& oid, IParam* param) override {
        descriptor_.addParam(oid, param);
    }

    /**
     * @brief get a constraint by oid
     */
    const catena::common::IConstraint* getConstraint() const override {
        return descriptor_.getConstraint();
    }

    /**
     * @brief add a constraint
     */
    void setConstraint(catena::common::IConstraint* constraint) override {
        descriptor_.setConstraint(constraint);
    }

    const std::string getScope() const override {
        return descriptor_.getScope();
    }

  private:
    Descriptor descriptor_;
    std::reference_wrapper<T> value_;
};

} // namespace lite
} // namespace catena
