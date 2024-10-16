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
 * @file AuthzInfo.h
 * @brief Class for handling authorization information
 * @author John R. Naylor john.naylor@rossvideo.com
 * @author John Danen
 * @date 2024-09-18
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <Path.h>
#include <Enums.h>

// lite
#include <ParamDescriptor.h>


namespace catena {
namespace lite {

/**
 * @brief Class for handling authorization information
 */
class AuthzInfo {
    using ParamDescriptor = catena::lite::ParamDescriptor;
  public:

    /**
     * @brief Construct a new AuthzInfo object
     * @param pd the ParamDescriptor of the object
     * @param scope the scope of the object
     */
    AuthzInfo(const ParamDescriptor& pd, const std::string& scope)
      : pd_{pd}, clientScope_{scope}{}

    /**
     * @brief Destroy the AuthzInfo object
     */
    virtual ~AuthzInfo() = default;

    /**
     * @brief Creates a AuthzInfo object for a sub parameter
     * @param oid the oid of the sub parameter
     * @return AuthzInfo the AuthzInfo object for the sub parameter
     */
    AuthzInfo subParamInfo(std::string oid) const {
        return AuthzInfo(pd_.getSubParam(oid), clientScope_);
    }

    /**
     * @brief Check if the client has read authorization
     * @return true if the client has read authorization
     */
    bool readAuthz() const {
        /**
         * @todo implement readAuthz
         */
        return true;
    }

    /**
     * @brief Check if the client has write authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz() const {
        if (pd_.readOnly()) {
            return false;
        }
        /**
         * @todo implement writeAuthz
         */
        return true;
    }

    const catena::common::IConstraint* getConstraint() const {
        return pd_.getConstraint();
    }

  private:
    const ParamDescriptor& pd_;
    const std::string clientScope_; /**< the scope of the object */
};

} // namespace lite
} // namespace catena