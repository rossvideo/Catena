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

//lite
#include <ParamDescriptor.h>
#include <AuthzInfo.h>  

using catena::lite::ParamDescriptor;

void ParamDescriptor::toProto(catena::Param &param, AuthzInfo& auth) const {
    param.set_type(type_);
    for (const auto& oid_alias : oid_aliases_) {
        param.add_oid_aliases(oid_alias);
    }
    for (const auto& [lang, text] : name_.displayStrings()) {
        (*param.mutable_name()->mutable_display_strings())[lang] = text;
    }
    param.set_widget(widget_);
    param.set_read_only(read_only_);
    if (constraint_) {
        constraint_->toProto(*param.mutable_constraint());
    }

    auto* dstParams = param.mutable_params();
    for (const auto& [oid, subParam] : subParams_) {
        AuthzInfo subAuth = auth.subParamInfo(oid);
        if (subAuth.readAuthz()) {
            subParam->toProto((*dstParams)[oid], subAuth);
        }
    }
}

const std::string& ParamDescriptor::name(const std::string& language) const { 
    auto it = name_.displayStrings().find(language);
    if (it != name_.displayStrings().end()) {
        return it->second;
    } else {
        static const std::string empty;
        return empty;
    }
}

const std::string ParamDescriptor::getScope() const {
      if (!scope_.empty()) {
        return scope_;
      } else if (parent_) {
        return parent_->getScope();
      } else {
        return dev_.getDefaultScope();
      }
    }