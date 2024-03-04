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

#include <string>
#include <assert.h>
#include <tuple>

template <> catena::Value::KindCase catena::getKindCase<int32_t>(const int32_t& src) {
    return catena::Value::KindCase::kInt32Value;
}
namespace catena {
namespace sdk {

/**
 * Parameter interface definition
 */
class IParam {
  public:
    /**
     * @brief Factory to make concrete objects derived from IParam
     */
    using Factory = catena::patterns::GenericFactory<IParam, catena::Value::KindCase, const catena::Param&>;

    using SharedIParam = std::shared_ptr<IParam>;
    using ParamsMap = std::unordered_map<std::string, SharedIParam>;

  public:
    /**
     * @brief Default destructor
     */
    virtual ~IParam() = default;

    /**
     * @brief Construct from a catena::Param (protobuf)
     */
    IParam(){};

    /**
     * @brief get the parameter's value as a catena::Value (protobuf)
     */
    virtual catena::Value& getValue(catena::Value& dst) const = 0;

    /**
     * @brief set the parameter's value from a catena::Value (protobuf)
     */
    virtual void setValue(const catena::Value& src) = 0;

    virtual void getValue(void* dst) const = 0;

    /**
     * @brief true if parameter needs to be included
     */
    virtual bool include() const = 0;

    /**
     * @brief marks the param as having been included.
     *
     * Subsequent calls to include() will return false.
     */
    virtual void included() = 0;

    virtual bool hasSubparams() const = 0;

    virtual ParamsMap& subparams() = 0;

    virtual IParam& operator=(const catena::Param& src) = 0;
};

/**
 * @brief output serialize operator for IParam
 */
inline catena::Value& operator<<(catena::Value& dst, const IParam& src) { return src.getValue(dst); }

inline const catena::Value& operator>>(const catena::Value& src, IParam& dst) {
    dst.setValue(src);
    return src;
}

class ParamCommon : public IParam {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     * @param oid object id
     * @param src source parameter
     */
    ParamCommon(const catena::Param& src) : IParam{}, type_{src.type()} {}

    /**
     * @brief Construct from an object id
     * @param oid object id
     * Note: type is set to UNDEFINED
     */
    explicit ParamCommon(const std::string& oid) : IParam{}, oid_{oid}, type_{catena::ParamType::UNDEFINED} {}

    virtual ~ParamCommon() = default;

  protected:
    std::string oid_;
    catena::ParamType type_;
    bool included_{false};
};

template <typename VT> class Param : public ParamCommon {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     * @param src source parameter
     */
    explicit Param(const catena::Param& src) : ParamCommon{src} {
        assert(src.value().kind_case() == catena::getKindCase(value_));
        auto& getter = catena::ParamAccessor::Getter::getInstance();
        getter[src.value().kind_case()](&value_, &src.value());
    }


    virtual ~Param() = default;

    Param(const Param&) = delete;
    Param& operator=(const Param&) = delete;

    catena::Value& getValue(catena::Value& dst) const override {
        auto& setter = catena::ParamAccessor::Setter::getInstance();
        setter[catena::getKindCase<VT>(value_)](&dst, &value_);
        return dst;
    }

    void setValue(const catena::Value& src) override {
        assert(src.kind_case() == catena::getKindCase(value_));
        auto& getter = catena::ParamAccessor::Getter::getInstance();
        getter[src.kind_case()](&value_, &src);
    }

    void getValue(void* dst) const override { *reinterpret_cast<VT*>(dst) = value_; }

    static bool registerWithFactory() {
        if (registered_) {
            return true;
        }
        auto& fac = IParam::Factory::getInstance();
        fac.addProduct(getKindCase<VT>(), [](const std::string& oid, const catena::Param& src) -> IParam* {
            return new Param<VT>(src);
        });
        return registered_;
    }

    bool hasSubparams() const override { return params_.size() > 0; }

    ParamsMap& subparams() override { return params_; }

    IParam& operator=(const catena::Param& src) override {
        return *this;
    }


  private:
    VT value_;
    static bool registered_;
    ParamsMap params_;
};

}  // namespace sdk
}  // namespace catena