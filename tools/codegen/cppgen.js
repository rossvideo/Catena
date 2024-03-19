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

'use strict';

const fs = require('fs');

const kCppTypes = {
    "INT32": "int32_t",
    "FLOAT32": "float",
    "STRING": "std::string",
}

function spaces(n) {
    return " ".repeat(n*2);
}

function initialCap(s) {
    return s.charAt(0).toUpperCase() + s.slice(1);
}

class CppGen {
    constructor(hloc, bloc, namespace) {
        this.hloc = hloc; // write a line of code to the header file
        this.bloc = bloc; // write a line of code to the body file
        this.namespace = namespace;
        this.convertors = {
            "STRUCT": (name, desc, indent = 0) => {
                const params = desc.params;
                hloc(`${spaces(indent)}struct ${initialCap(name)} {`);
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
                        this.convertors.STRUCT(userDefinedType, params[p], indent + 1);
                    }
                    ++n;
                }
                for (let i = 0; i < n; ++i) {
                    hloc(`${spaces(indent+1)}${types[i]} ${names[i]};`);
                }
                hloc(`${spaces(indent+1)}using FieldTypes = TypeList<${types.join(', ')}>;`);
                hloc(`${spaces(indent+1)}static constexpr std::string fieldNames[] = {${names.map(n => `"${n}"`).join(', ')}};`);
                hloc(`${spaces(indent)}};`);
            }
        };
    }
    
    convert (oid, desc) {
        if (desc.type in this.convertors) {
            return this.convertors[desc.type](oid, desc);
        } else {
            console.log(`No convertor found for ${oid} of type ${desc.type}`);
        }
    }
};

module.exports = CppGen;

