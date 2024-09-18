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
#include <PolyglotText.h>

// meta
#include <meta/IsVector.h>

// protobuf interface
#include <interface/param.pb.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <cassert>

namespace catena {
namespace lite {

template <typename T>
class ParamWithValue; // forward declaration
class AuthzInfo; // forward declaration

/**
 * @brief ParamDescriptor provides information about a parameter
 */
class ParamDescriptor {
  using OidAliases = std::vector<std::string>;

  public:
    /**
     * @brief ParamDescriptor does not have a default constructor
     */
    ParamDescriptor() = delete;

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
    ParamDescriptor(
      catena::ParamType type, 
      const OidAliases& oid_aliases, 
      const PolyglotText::ListInitializer name, 
      const std::string& widget,
      const std::string& scope, 
      const bool read_only, 
      const std::string& oid, 
      ParamDescriptor* parent,
      Device& dev)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, scope_{scope}, read_only_{read_only},
        constraint_{nullptr}, parent_{parent}, dev_{dev} {
      setOid(oid);
      parent_->addSubParam(oid, this);
    }

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
    template <typename T>
    ParamDescriptor(
      catena::ParamType type, 
      const OidAliases& oid_aliases, 
      const PolyglotText::ListInitializer name, 
      const std::string& widget,
      const std::string& scope, 
      const bool read_only, 
      const std::string& oid, 
      ParamWithValue<T>* parent,
      Device& dev)
      : type_{type}, oid_aliases_{oid_aliases}, name_{name}, widget_{widget}, scope_{scope}, read_only_{read_only},
        constraint_{nullptr}, dev_{dev} {
      setOid(oid);
      parent->addParam(oid, this);
    }

    /**
     * @brief get the parameter type
     */
    ParamType type() const { return type_; }

    /**
     * @brief get the parameter name
     */
    const PolyglotText::DisplayStrings& name() const { return name_.displayStrings(); }

    const std::string& getOid() const { return oid_; }

    void setOid(const std::string& oid) { oid_ = oid; }

    inline bool readOnly() const { return read_only_; }

    inline void readOnly(bool flag) { read_only_ = flag; }
    
    void toProto(catena::Param &param, AuthzInfo& auth) const;


    /**
     * @brief get the parameter name by language
     * @param language the language to get the name for
     * @return the name in the specified language, or an empty string if the language is not found
     */
    const std::string& name(const std::string& language) const;
    /**
     * @brief add an item to one of the collections owned by the device
     * @tparam TAG identifies the collection to which the item will be added
     * @param key item's unique key
     * @param item the item to be added
     */
    void addSubParam(const std::string& oid, ParamDescriptor* item) {
      subParams_[oid] = item;
    }

    /**
     * @brief gets a sub parameter's paramDescriptor
     * @return ParamDescriptor of the sub parameter
     */
    ParamDescriptor& getSubParam(const std::string& oid) const {
      return *subParams_.at(oid);
    }

    /**
     * @brief get a constraint by oid
     */
    const catena::common::IConstraint* getConstraint() const {
      return constraint_;
    }

    /**
     * @brief add a constraint
     */
    void setConstraint(catena::common::IConstraint* constraint) {
      constraint_ = constraint;
    }

    const std::string getScope() const;

  private:
    ParamType type_;  // ParamType is from param.pb.h
    std::vector<std::string> oid_aliases_;
    PolyglotText name_;
    std::string widget_;
    std::string scope_;
    bool read_only_;
    std::unordered_map<std::string, ParamDescriptor*> subParams_;
    std::unordered_map<std::string, catena::common::IParam*> commands_;
    common::IConstraint* constraint_;
    
    std::string oid_;
    ParamDescriptor* parent_;
    const Device& dev_;
};

}  // namespace lite
}  // namespace catena
