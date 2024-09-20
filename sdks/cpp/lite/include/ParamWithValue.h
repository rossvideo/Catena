#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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

    void fromProto(catena::Value& value) override {
        
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
    catena::common::IConstraint* getConstraint(const std::string& oid) override {
        return descriptor_.getConstraint(oid);
    }

    /**
     * @brief add a constraint
     */
    void addConstraint(const std::string& oid, catena::common::IConstraint* constraint) override {
        descriptor_.addConstraint(oid, constraint);
    }

  private:
    Descriptor descriptor_;
    std::reference_wrapper<T> value_;
};

} // namespace lite
} // namespace catena
