#pragma once

#include <lite/include/IParam.h>
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>
#include <lite/include/PolyglotText.h>
#include <lite/include/Tags.h>

#include <lite/param.pb.h>

#include <vector>
#include <string>

namespace catena {
namespace lite {



/**
 * @brief ParamDescriptor provides information about a parameter
 * @tparam T the parameter's value type
 */
template <typename T> class ParamDescriptor : public catena::common::IParam {
  public:
    

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
    }

    /**
     * @brief gets an item from one of the collections owned by the device
     * @return nullptr if the item is not found, otherwise the item
     */
    template <typename TAG>
    typename TAG::type* getItem(const std::string& key) const {
        GET_ITEM(common::ParamTag, params_)
        GET_ITEM(common::CommandTag, commands_)
        return nullptr;
    }

    /**
     * @brief get a child parameter by name
     */
    IParam* getParam(const std::string& oid) override {
        return getItem<common::ParamTag>(oid);
    }


    void addParam(const std::string& oid, IParam* param) override {
        addItem<common::ParamTag>(oid, param);
    }

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::string widget_;
    bool read_only_;
    std::unordered_map<std::string, IParam*> params_;
    std::unordered_map<std::string, IParam*> commands_;

    
    std::string oid_;
    IParam* parent_;
};

}  // namespace lite
}  // namespace catena
