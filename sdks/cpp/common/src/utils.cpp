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

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <utils.h>

#include <cassert>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

std::string catena::printJSON(const google::protobuf::Message &m) {
  std::string str;
  google::protobuf::util::JsonPrintOptions jopts{};
  jopts.add_whitespace = true;
  jopts.preserve_proto_field_names = true;
  google::protobuf::util::MessageToJsonString(m, &str, jopts);
  return str;
}

void catena::parseJSON(const std::string &msg, google::protobuf::Message &m) {
  google::protobuf::util::JsonParseOptions jopts{};
  google::protobuf::util::JsonStringToMessage(msg, &m, jopts);
}

std::string catena::readFile(std::filesystem::path path) {
  std::ifstream file(path, std::ios::in | std::ios::binary); // lock the file
  const auto sz = std::filesystem::file_size(path);          // get its size
  std::string ans(sz, '\0'); // create our buffer
  file.read(ans.data(), sz); // read the file
  return ans; // return the result - copy elision should prevent copy ctor
              // being called
}

/**
 * @brief Substitutes all occurences of one char sequence in a string with
 * another.
 *
 * @param str in/out the string to work on, done in place
 * @param seq in sequence to match
 * @param rep in sequence to replace the match
 */
void catena::subs(std::string &str, const std::string &seq,
                  const std::string &rep) {
  std::ostringstream oss;
  std::size_t prev{}, pos{};
  while (true) {
    prev = pos;
    pos = str.find(seq, pos); // locate target sequence in input
    if (pos == std::string::npos) {
      // i.e. not found
      break; // exit loop
    }
    oss << str.substr(prev, pos - prev); // write what came before
    oss << rep;                          // write the replacement
    pos += seq.size();                   // skip over the target sequence
  }
  oss << str.substr(prev); // write to the end of the input string
  str = oss.str();
}
