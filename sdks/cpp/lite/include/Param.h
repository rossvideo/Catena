#pragma once

/**
 * @file Param.h
 * @brief Param provides convenient access to parameter values
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-07-07
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <lite/include/IParam.h>
#include <lite/include/Device.h>
#include <lite/include/StructInfo.h>
#include <lite/include/PolyglotText.h>
#include <lite/include/Collection.h>
#include <common/include/IConstraint.h>

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
template <typename T> 
class Param : public IParam {
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
    Param(catena::ParamType type, T& value, const OidAliases& oid_aliases, const PolyglotText::ListInitializer name,
        const std::string& widget, const bool read_only, const std::string& oid, Collection<IParam>& device_params)
        : type_{type}, value_{value}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget} {
        setOid(oid);
        device_params.addItem(oid, this);
    }

    /**
     * @brief get the value of the parameter
     */
    inline T& get() { return value_.get(); }

    /**
     * @brief serialize the parameter value to protobuf
     * @param dst the protobuf value to serialize to
     */
    void toProto(catena::Value& dst) const override;

    /**
     * @brief deserialize the parameter value from protobuf
     * @param src the protobuf value to deserialize from
     * @note this method may constrain the source value and modify it
     */
    void fromProto(catena::Value& src) override;

    /**
     * @brief serialize the parameter descriptor to protobuf
     * @param param the protobuf param to serialize to
     */
    void toProto(catena::Param& param) const override {
        // type member
        param.set_type(type_());

        // oid_aliases member
        param.mutable_oid_aliases()->Reserve(oid_aliases_.size());
        for (const auto& oid_alias : oid_aliases_) {
            param.add_oid_aliases(oid_alias);
        }

        // name member
        catena::PolyglotText name_proto;
        for (const auto& [lang, text] : name_.displayStrings()) {
            (*name_proto.mutable_display_strings())[lang] = text;
        }
        param.mutable_name()->Swap(&name_proto);

        // widget member
        param.set_widget(widget_);

        // constraint member
        /// @todo need to update for when more constraints are added 
        if (constraints_.empty()) {
            constraints_.first()->toProto(*param.mutable_constraint());
        }

        // value member
        toProto(*param.mutable_value());
    }

    /**
     * @brief get the parameter type
     * @return the parameter type
     */
    ParamType type() const override { return type_; }

    /**
     * @brief get the parameter name
     * @return map of languages and corresponding names
     */
    const PolyglotText::DisplayStrings& name() const { return name_.displayStrings(); }

    /**
     * @brief return the read-only status of the parameter
     */
    const bool isReadOnly() const override { return read_only_; }

    void setReadOnly(bool read_only) { read_only_ = read_only; }

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
     * @brief get a collection of items from this param
     * @param tag the tag of the collection to get
     * @return collection of items
     * @note tag can be ConstraintTag
     */
    template <typename TAG>
    Collection<typename TAG::type>& getCollection(TAG tag) {
      if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        return constraints_;
      }
    }

    /**
     * @brief retreive an item from the param by name
     * @param name path to the item relative to device
     * @return item can be IConstraint
     * @note tag can be ConstraintTag
     */
    template <typename TAG>
    TAG::type* getItem(const std::string& name, TAG tag) const {
      if constexpr(std::is_same_v<TAG, ConstraintTag>) {
        return constraints_.getItem(name);
      } else {
        return nullptr;
      }
    }

private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    Collection<catena::common::IConstraint> constraints_;
    std::reference_wrapper<T> value_;
    // std::reference_wrapper<Device> dm_;
    std::string widget_;
    bool read_only_;
};

}  // namespace lite
}  // namespace catena
