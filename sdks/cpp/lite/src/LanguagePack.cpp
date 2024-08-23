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

// lite
#include <LanguagePack.h>

using namespace catena::lite;
using catena::common::LanguagePackTag;

LanguagePack::LanguagePack(const std::string& name, ListInitializer list, Device& dev)
    : name_{name}, words_(list.begin(), list.end()) {
    dev.addItem<LanguagePackTag>(name, this);
}

void LanguagePack::fromProto(const catena::LanguagePack& pack) {
    name_ = pack.name();
    for (const auto& [key, value] : pack.words()) {
        words_[key] = value;
    }
}

void LanguagePack::toProto(catena::LanguagePack& pack) const {
    pack.set_name(name_);
    for (const auto& [key, value] : words_) {
        (*pack.mutable_words())[key] = value;
    }
}



