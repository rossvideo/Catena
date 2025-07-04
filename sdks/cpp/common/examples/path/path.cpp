/*
 * Copyright 2024 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
//

/**
 * @brief Example program to demonstrate the Path class
 * @file path.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// common
#include <Path.h>

#include <iostream>
#include <memory>
#include <utility>

using catena::common::Path;
using std::cout;
using std::endl;

void document(Path& p) {
    cout << "path: " << p.fqoid() << "\nhas length: " << p.size() << endl;
    while (p.size()) {
        if (p.front_is_index()) {
            Path::Index idx = p.front_as_index();
            cout << "segment " << p.walked() << " has type Index and value: ";
            if (idx == Path::kEnd) {
              cout << "kEnd";
            } else {
              cout << idx;
            }
            cout << endl;
        }
        if (p.front_is_string()) {
            cout << "segment " << p.walked() << " has type string and value: " << p.front_as_string() << endl;
        }
        p.pop();
    }
}

int main() {
    Path top_level_oid("/top_level_oid");
    document(top_level_oid);

    Path top_level_array_element("/top_level_array/3");
    document(top_level_array_element);

    Path nested_struct("/parent/child/grandchild");
    document(nested_struct);

    // demos the one-past-the-end element accessor
    Path struct_array("/parent/-/child");
    document(struct_array);

    // The document method consumes the Path passed to it
    cout << "\njptr should be empty" << endl;
    document(struct_array);
    
    // we can reverse this 2 ways - reverse the last pop operation
    cout << "\njptr should have its last segment" << endl;
    struct_array.unpop();
    document(struct_array);

    // or rewind to the very beginning
    cout << "\njptr should be fully restored" << endl;
    struct_array.rewind();
    document(struct_array);

    Path struct_array_element_field("/parent/3/child");
    document(struct_array_element_field);
}

/* Possible Output
path: /top_level_oid
has length: 1
segment 0 has type string and value: top_level_oid
path: /top_level_array/3
has length: 2
segment 0 has type string and value: top_level_array
segment 1 has type Index and value: 3
path: /parent/child/grandchild
has length: 3
segment 0 has type string and value: parent
segment 1 has type string and value: child
segment 2 has type string and value: grandchild
path: /parent/-/child
has length: 3
segment 0 has type string and value: parent
segment 1 has type Index and value: kEnd
segment 2 has type string and value: child

jptr should be empty
path: /parent/-/child
has length: 0

jptr should have its last segment
path: /parent/-/child
has length: 1
segment 2 has type string and value: child

jptr should be fully restored
path: /parent/-/child
has length: 3
segment 0 has type string and value: parent
segment 1 has type Index and value: kEnd
segment 2 has type string and value: child
path: /parent/3/child
has length: 3
segment 0 has type string and value: parent
segment 1 has type Index and value: 3
segment 2 has type string and value: child
*/