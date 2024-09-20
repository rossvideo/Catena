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
 * @file ParamWithDescription.h
 * @brief Interface for accessing parameter descriptions
 * @author John R. Naylor
 * @date 2024-08-20
 */

//common
#include <Tags.h>
#include <IParam.h>
#include <IConstraint.h>

// lite
#include <Device.h>
#include <StructInfo.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

#include <vector>
#include <string>

namespace catena {
namespace lite {


/**
 * @brief ParamDescriptor provides information about a parameter
 * @tparam T the parameter's value type
 */
template <typename T> 
class ParamDescriptor : public catena::common::IParam {
  public:
    /**
     * @brief ParamDescriptor does not have a default constructor
     */
    ParamDescriptor() = delete;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    ParamDescriptor(T& value) = delete;

    /**
     * @brief ParamDescriptor does not have copy semantics
     */
    T& operator=(const T& value) = delete;

    /**
     * @brief ParamDescriptor has move semantics
     */
    ParamDescriptor(ParamDescriptor&& value) = default;

    /**
     * @brief ParamDescriptor has move semantics
     */
    ParamDescriptor& operator=(ParamDescriptor&& value) = default;

    /**
     * @brief default destructor
     */
    virtual ~ParamDescriptor() = default;

    /**
     * @brief the main constructor
     * @param type the parameter type
     * @param oid_aliases the parameter's oid aliases
     * @param name the parameter's name
     * @param widget the parameter's widget
     * @param read_only the parameter's read-only status
     * @param oid the parameter's oid
     * @param collection the parameter belongs to
     */
    ParamDescriptor(catena::ParamType type, const OidAliases& oid_aliases, const PolyglotText::ListInitializer name, const std::string& widget,
      const bool read_only, const std::string& oid, Device &dev)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, read_only_{read_only}, parent_{nullptr} {
      setOid(oid);
      dev.addItem<common::ParamTag>(oid, this);
    }

    ParamDescriptor(catena::ParamType type, const OidAliases& oid_aliases, const PolyglotText::ListInitializer name, const std::string& widget,
      const bool read_only, const std::string& oid, IParam* parent)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, read_only_{read_only}, parent_{parent} {
      setOid(oid);
      parent_->addParam(oid, this);
    }

    /**
     * @brief get the parameter type
     */
    ParamType type() const override { return type_; }

    /**
     * @brief get the parameter name
     */
    const PolyglotText::DisplayStrings& name() const { return name_.displayStrings(); }

    const std::string& getOid() const override { return oid_; }

    void setOid(const std::string& oid) override { oid_ = oid; }

    inline bool readOnly() const override { return read_only_; }

    inline void readOnly(bool flag) override { read_only_ = flag; }

    void toProto(catena::Value& value) const override {
      //catena::lite::toProto<T>(value, &value_.get());
    }

    void fromProto(catena::Value& value) override {
      //catena::lite::fromProto<T>(&value_.get(), value);
    }
    
    void toProto(catena::Param &param) const override {
      param.set_type(type_());
      for (const auto& oid_alias : oid_aliases_) {
        param.add_oid_aliases(oid_alias);
      }
      for (const auto& [lang, text] : name_.displayStrings()) {
        (*param.mutable_name()->mutable_display_strings())[lang] = text;
      }
      param.set_widget(widget_);
      param.set_read_only(read_only_);
      toProto(*param.mutable_value());
    }


    /**
     * @brief get the parameter name by language
     * @param language the language to get the name for
     * @return the name in the specified language, or an empty string if the language is not found
     */
    const std::string& name(const std::string& language) const { 
      auto it = name_.displayStrings().find(language);
      if (it != name_.displayStrings().end()) {
        return it->second;
      } else {
        static const std::string empty;
        return empty;
      }
    }

    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    template <typename TAG>
    void addItem(const std::string& key, typename TAG::type* item) {
      if constexpr (std::is_same_v<TAG, common::ParamTag>) {
        params_[key] = item;
      }
      if constexpr (std::is_same_v<TAG, common::CommandTag>) {
        commands_[key] = item;
      }
      if constexpr (std::is_same_v<TAG, common::ConstraintTag>) {
        constraints_[key] = item;
      }
    }

    /**
     * @brief gets an item from one of the collections owned by the device
     * @return nullptr if the item is not found, otherwise the item
     */
    template <typename TAG>
    typename TAG::type* getItem(const std::string& key) const {
      GET_ITEM(common::ParamTag, params_)
      GET_ITEM(common::CommandTag, commands_)
      GET_ITEM(common::ConstraintTag, constraints_)
      return nullptr;
    }

    /**
     * @brief get a child parameter by name
     */
    IParam* getParam(const std::string& oid) override {
      return getItem<common::ParamTag>(oid);
    }

    /**
     * @brief add a child parameter
     */
    void addParam(const std::string& oid, IParam* param) override {
      addItem<common::ParamTag>(oid, param);
    }

    /**
     * @brief get a constraint by oid
     */
    catena::common::IConstraint* getConstraint(const std::string& oid) override {
      return getItem<common::ConstraintTag>(oid);
    }

    /**
     * @brief add a constraint
     */
    void addConstraint(const std::string& oid, catena::common::IConstraint* constraint) override {
      addItem<common::ConstraintTag>(oid, constraint);
    }

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::string widget_;
    bool read_only_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, IParam*> commands_;
    std::unordered_map<std::string, catena::common::IConstraint*> constraints_;
    
    std::string oid_;
    IParam* parent_;
};

}  // namespace lite
}  // namespace catena
