/**
 * @brief Analog to catena::Import
 * @file Import.cpp
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

#include <Import.h>

using catena::sdk::Import;

Import::Import(const catena::Import& src) : url_{src.url()}, digest_{src.digest()}, method_{src.method()} {}

Import& Import::operator=(const catena::Import& src) {
    url_ = src.url();
    digest_ = src.digest();
    method_ = src.method();
    return *this;
}

bool Import::isIncludeDirective() const { return url_.compare("include") == 0; }