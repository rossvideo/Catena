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
#include <Path.h>

// lite
#include <ParamDescriptor.h>
#include <Device.h>
#include <StructInfo.h>
#include <PolyglotText.h>
#include <AuthzInfo.h>

// protobuf interface
#include <interface/param.pb.h>

#include <functional>
#include <string>
#include <memory>

namespace catena {
namespace lite {

template <typename T>
class ParamWithValue : public catena::common::IParam {
  public:
    ParamWithValue() = delete;

    /**
     * @brief Construct a new ParamWithValue object and add it to the device
     */
    ParamWithValue(
        T& value,
        ParamDescriptor& descriptor,
        Device &dev
    ) : value_{value}, descriptor_{descriptor} {
        dev.addItem<common::ParamTag>(descriptor.getOid(), this);
    }

    /**
     * @brief Construct a new ParamWithValue object without adding it to device
     */
    ParamWithValue(
        T& value,
        ParamDescriptor& descriptor
    ) : value_{value}, descriptor_{descriptor} {}

    template <typename FieldType = T, typename ParentType>
    ParamWithValue(
        const FieldInfo<FieldType, ParentType>& field, 
        ParentType& parentValue,
        ParamDescriptor& parentDescriptor,
        const std::string& oid
    ) : descriptor_{parentDescriptor.getSubParam(oid)}, value_{(parentValue.*(field.memberPtr))} {}

    ParamWithValue(const ParamWithValue&) = delete;
    ParamWithValue& operator=(const ParamWithValue&) = delete;
    ParamWithValue(ParamWithValue&&) = default;
    ParamWithValue& operator=(ParamWithValue&&) = default;
    virtual ~ParamWithValue() = default;

    std::unique_ptr<IParam> copy() const override {
        return std::make_unique<ParamWithValue<T>>(value_.get(), descriptor_);
    }

    void toProto(catena::Value& value, std::string& clientScope) const override {
        AuthzInfo auth(descriptor_, clientScope);
        if (auth.readAuthz()) {
            catena::lite::toProto<T>(value, &value_.get(), auth);
        }
    }

    /**
     * @brief serialize the parameter descriptor to protobuf
     * include both the descriptor and the value
     * @param param the protobuf value to serialize to
     */
    void toProto(catena::Param& param, std::string& clientScope) const override {
        AuthzInfo auth(descriptor_, clientScope);
        if (auth.readAuthz()) {
            descriptor_.toProto(param, auth);        
            catena::lite::toProto<T>(*param.mutable_value(), &value_.get(), auth);
        }
    }

    // void fromProto(catena::Value& value) override {
    //     catena::lite::fromProto<T>(&value_.get(), value);
    // }

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
    std::unique_ptr<IParam> getParam(Path oid) override {
        return getParam(oid, value_.get());
    }

  private:
    template <typename U>
    std::unique_ptr<IParam> getParam(Path oid, U& value) {
        // This type is not a CatenaStruct so it has no sub-params
        return nullptr;
    }

    template <CatenaStruct U>
    std::unique_ptr<IParam> getParam(Path oid, U& value) {
        catena::common::Path::Segment segment = oid.front();
        std::string* oidStr = std::get_if<std::string>(&segment);
        if (!oidStr) return nullptr;
        oid.pop_front();
        auto fields = Reflect<U>::fields;

        std::size_t index = findIndexByName<U>(fields, *oidStr);
        std::unique_ptr<IParam> ip = getTupleElement(fields, index, *oidStr);
        if (oid.empty()) {
            return ip;
        } else {
            if (!ip) return nullptr;
            return ip->getParam(oid);
        }
    }

    // Helper function to get tuple element by runtime index
    template <std::size_t I, typename Tuple>
    std::unique_ptr<IParam> getTupleElementImpl(const Tuple& tuple, std::size_t index, const std::string& oid) {
        if constexpr (I < std::tuple_size_v<Tuple>) {
            if (I == index) {
                using FieldType = typename std::tuple_element_t<I, Tuple>::Field;
                return std::make_unique<ParamWithValue<FieldType>>(std::get<I>(tuple), value_.get(), descriptor_, oid);
            } else {
                return getTupleElementImpl<I + 1>(tuple, index, oid);
            }
        } else {
            return nullptr;
        }
    }

    // Primary function to get tuple element by runtime index
    template <typename Tuple>
    std::unique_ptr<IParam> getTupleElement(const Tuple& tuple, std::size_t index, const std::string& oid) {
        return getTupleElementImpl<0>(tuple, index, oid);
    }


  public:
    /**
     * @brief add a child parameter
     */
    void addParam(const std::string& oid, ParamDescriptor* param) {
        descriptor_.addSubParam(oid, param);
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

  public:
    const std::string getScope() const override {
        return descriptor_.getScope();
    }

  private:
    ParamDescriptor& descriptor_;
    std::reference_wrapper<T> value_;
};

} // namespace lite
} // namespace catena
