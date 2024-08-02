#pragma once

#include <lite/include/IParam.h>
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>
#include <lite/include/PolyglotText.h>

#include <lite/param.pb.h>

#include <functional>  // reference_wrapper
#include <vector>
#include <string>

namespace catena {
namespace lite {



/**
 * @brief Param provides convenient access to parameter values
 * @tparam T the parameter's value type
 */
template <typename T> class Param : public IParam {
  public:
    /**
     * @brief OidAliases is a vector of strings
     */
    using OidAliases = std::vector<std::string>;

  public:
    /**
     * @brief Param does not have a default constructor
     */
    Param() = delete;

    /**
     * @brief Param does not have copy semantics
     */
    Param(T& value) = delete;

    /**
     * @brief Param does not have copy semantics
     */
    T& operator=(const T& value) = delete;

    /**
     * @brief Param has move semantics
     */
    Param(Param&& value) = default;

    /**
     * @brief Param has move semantics
     */
    Param& operator=(Param&& value) = default;

    /**
     * @brief default destructor
     */
    virtual ~Param() = default;

    /**
     * @brief the main constructor
     */
    Param(catena::ParamType type, T& value, const OidAliases& oid_aliases, const PolyglotText::ListInitializer name, const std::string& widget,
          const bool read_only, const std::string& oid, Device& dm)
        : type_{type}, value_{value}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, read_only_{read_only}, dm_{dm} {
        setOid(oid);
        dm.addItem<Device::ParamTag>(oid, this, Device::ParamTag{});
    }

    /**
     * @brief get the value of the parameter
     */
    inline T& get() { return value_.get(); }

    /**
     * @brief serialize the parameter value to protobuf
     */
    void toProto(catena::Value& value) const override;

    /**
     * @brief deserialize the parameter value from protobuf
     */
    void fromProto(const catena::Value& value) override;

    /**
     * @brief serialize the parameter descriptor to protobuf
     */
    void toProto(catena::Param& param) const override {
        param.set_type(type_());
        param.mutable_oid_aliases()->Reserve(oid_aliases_.size());
        for (const auto& oid_alias : oid_aliases_) {
            param.add_oid_aliases(oid_alias);
        }
        catena::PolyglotText name_proto;
        for (const auto& [lang, text] : name_.displayStrings()) {
            (*name_proto.mutable_display_strings())[lang] = text;
        }
        param.mutable_name()->Swap(&name_proto);
        param.set_widget(widget_);
        toProto(*param.mutable_value());
    }

    /**
     * @brief get the parameter type
     */
    ParamType type() const override { return type_; }

    /**
     * @brief get the parameter name
     */
    const PolyglotText::DisplayStrings& name() const { return name_.displayStrings(); }

    /**
     * @brief return the read-only status of the parameter
     */
    const bool readOnly() const override { return read_only_; }

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

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::reference_wrapper<T> value_;
    std::reference_wrapper<Device> dm_;
    std::string widget_;
    const bool read_only_;
};

}  // namespace lite
}  // namespace catena
