/*
Copyright © 2024 Ross Video Limited, All Rights Reserved.
 
Licensed under the Creative Commons Attribution NoDerivatives 4.0
International Licensing (CC-BY-ND-4.0);
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:
 
https://creativecommons.org/licenses/by-nd/4.0/
 
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

const kCppTypes = {
    "INT32": "int32_t",
    "FLOAT32": "float",
    "STRING": "std::string",
}

'use strict';

function spaces(n) {
    return " ".repeat(n*2);
}

function initialCap(s) {
    return s.charAt(0).toUpperCase() + s.slice(1);
}

let convertors = {
    "STRUCT": (name, body, indent = 0) => {
        const params = body.params;
        console.log(`${spaces(indent)}struct ${initialCap(name)} {`);
        let n = 0;
        let types = [];
        let names = [];
        for (p in params) {
            const type = params[p].type;
            names.push(p);
            
            if (type in kCppTypes) {
                let cppType = kCppTypes[type];
                types.push(cppType);
            } else if (type === "STRUCT") {
                let userDefinedType = p.charAt(0).toUpperCase() + p.slice(1);
                types.push(userDefinedType);
                convertors.STRUCT(userDefinedType, params[p], indent + 1);
            }
            ++n;
        }
        for (i = 0; i < n; ++i) {
            console.log(`${spaces(indent+1)}${types[i]} ${names[i]};`);
        }
        console.log(`${spaces(indent+1)}using FieldTypes = TypeList<${types.join(', ')}>;`);
        console.log(`${spaces(indent+1)}static constexpr std::string fieldNames[] = {${names.map(n => `"${n}"`).join(', ')}};`);
        console.log(`${spaces(indent)}};`);
    }
}

module.exports = function convert(name, body) {
    if (body.type in convertors) {
        return convertors[body.type](name, body);
    } else {
        console.log(`No convertor found for ${name} of type ${body.type}`);
    }
}
