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

#include <Functory.h>
#include <Path.h>
#include <Status.h>
#include <TypeTraits.h>
#include <ParamAccessor.h>

#include <string>
#include <assert.h>

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

    virtual const std::string& getOid() = 0;

    virtual void setOid(const std::string& oid) = 0;
};

/** 
 * @brief output serialize operator for IParam
*/
inline catena::Value& operator<<(catena::Value& dst, const IParam& src) {
    return src.getValue(dst);
}

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
    ParamCommon(const std::string& oid, const catena::Param& src) : IParam{}, oid_{oid}, type_{src.type()} {}

    /**
     * @brief Construct from an object id
     * @param oid object id
     * Note: type is set to UNDEFINED
     */
    explicit ParamCommon(const std::string& oid) : IParam{}, oid_{oid}, type_{catena::ParamType::UNDEFINED} {}

    virtual ~ParamCommon() = default;

    const std::string& getOid() override { return oid_; }

    void setOid(const std::string& oid) override { oid_ = oid; }

  protected:
    std::string oid_;
    catena::ParamType type_;
};

template <typename VT> class Param : public ParamCommon {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     * @param oid object id
     * @param src source parameter
     */
    Param(const std::string& oid, const catena::Param& src) : ParamCommon{oid, src} {
        assert(src.value().kind_case() == catena::getKindCase(value_));
        auto& getter = catena::ParamAccessor::Getter::getInstance();
        getter[src.value().kind_case()](&value_, &src.value());
    }

    /**
     * @brief Construct from an object id
     * @param oid object id
    */
    explicit Param(const std::string& oid) : ParamCommon{oid} {}

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

  private:
    VT value_;
};

}  // namespace sdk
}  // namespace catena