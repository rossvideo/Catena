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


#include <param.pb.h>

#include <string>

namespace catena {
namespace sdk {
class Import {
  public:
    using DigestAlgorithm = catena::Import_DigestAlgorithm;

  public:
    Import() = default;
    virtual ~Import() = default;

    Import(const Import&) = delete;
    Import& operator=(const Import&) = delete;

    Import(Import&&) = default;
    Import& operator=(Import&&) = default;

    Import(const catena::Import& src);

    Import& operator=(const catena::Import& src);

    bool isIncludeDirective() const;

  private:
    std::string url_;
    std::string digest_;
    DigestAlgorithm method_;
};
}  // namespace sdk
}  // namespace catena
