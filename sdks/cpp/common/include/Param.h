#pragma once
/**
 * @brief Param analog to catena::Param
 * @file Param.h
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

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

#include <constraint.pb.h>
#include <device.pb.h>
#include <param.pb.h>

#include <GenericFactory.h>
#include <Path.h>
#include <Status.h>
#include <TypeTraits.h>
#include <ParamAccessor.h>
#include <Import.h>
#include <Meta.h>
#include <IParam.h>

#include <string>
#include <assert.h>
#include <tuple>
#include <type_traits>
#include <unordered_map>


namespace catena {
namespace sdk {


class ParamCommon : public IParam {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     * @param oid object id
     * @param src source parameter
     */
    ParamCommon(const catena::Param& src);

    ParamCommon& operator=(const catena::Param& src) override;

    /**
     * @brief Construct from an object id
     * @param oid object id
     * Note: type is set to UNDEFINED
     */

    virtual ~ParamCommon() = default;

    bool include() const override;

    void included() override;

  protected:
    bool included_{false};
    catena::ParamType type_;
    sdk::Import import_;
};

template <typename VT> class Param : public ParamCommon {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     * @param src source parameter
     */
    explicit Param(const catena::Param& src) : ParamCommon{src} {
        const catena::Value& value = src.value();
        catena::Value::KindCase kind = value.kind_case();
        catena::Value::KindCase expected = catena::getKindCase(meta::TypeTag<VT>{});
        assert(kind == expected);
        auto& getter = catena::Getter::getInstance();
        if constexpr(has_getStructInfo<VT>) {
            const StructInfo& si = VT::getStructInfo();
            char* base = reinterpret_cast<char*>(&value_); // necessary for correct pointer arithmetic
            for (const auto& field : si.fields) {
                auto& fields = value.struct_value().fields();
                if (fields.contains(field.name)) {
                    // skip value assignment if field is not present
                    const catena::Value& field_value = fields.at(field.name).value();
                    getter[field_value.kind_case()](base + field.offset, &field_value);
                }
                // construct the subparam
                params_[field.name].reset(field.makeParam(src.params().at(field.name)));
            }
        } else {
            getter[kind](&value_, &value);
        }
    }

    virtual ~Param() = default;

    Param(const Param&) = delete;
    Param& operator=(const Param&) = delete;

    catena::Value& getValue(catena::Value& dst) const override {
        auto& setter = catena::Setter::getInstance();
        setter[catena::getKindCase(meta::TypeTag<VT>{})](&dst, &value_);
        return dst;
    }


    void setValue(const catena::Value& src) override {
        assert(src.kind_case() == catena::getKindCase(meta::TypeTag<VT>{}));
        auto& getter = catena::Getter::getInstance();
        if constexpr (has_getStructInfo<VT>) {
            const StructInfo& si = VT::getStructInfo();
            void* base = &value_;
            for (const auto& field : si.fields) {
                src.struct_value().fields().at(field.name);
            }
        } else {
            getter[src.kind_case()](&value_, &src);
        }
    }

    void getValue(void* dst) const override { *reinterpret_cast<VT*>(dst) = value_; }

    bool hasSubparams() const override { return params_.size() > 0; }

    ParamsMap& subparams() override { return params_; }

    Param& operator=(const catena::Param& src) override { return *this; }

    /**
     * @brief Register the Param with the factory
     */
    static bool registerWithFactory() {
        static bool registered_ = false;
        if (registered_) {
            return true;
        }
        auto& fac = IParam::Factory::getInstance();
        fac.addProduct(getKindCase<VT>(meta::TypeTag<VT>{}),
                       [](const catena::Param& src) -> IParam* { return new Param<VT>(src); });
        return registered_ = true;
    }

  private:
    VT value_;
    ParamsMap params_;
};


}  // namespace sdk
}  // namespace catena