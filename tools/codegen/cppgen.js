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
                const classname = initialCap(name);
                const fqname = `${namespace}::${classname}`;
                hloc(`struct ${classname} {`, indent);
                let n = 0;
                let types = [];
                let names = [];
                // gather information about the struct
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

                // write the struct to the header file
                for (let i = 0; i < n; ++i) {
                    hloc(`${types[i]} ${names[i]};`, indent+1);
                }
                hloc(`static const catena::StructInfo& getStructInfo();`, indent+1)
                hloc(`};`, indent);

                // write the getStructInfo method to the body file
                let bodyIndent = 0;
                bloc(`const catena::StructInfo& ${fqname}::getStructInfo() {`, bodyIndent);
                bloc(`static catena::StructInfo t;`, bodyIndent+1);
                bloc(`if (t.name.length()) return t;`, bodyIndent+1);
                bloc(`t.name = "${name}";`, bodyIndent+1);
                bloc(`catena::FieldInfo fi;`, bodyIndent+1);
                for (let i = 0; i < n; ++i) {
                    let indent = bodyIndent+2;
                    bloc(`// register info for the ${names[i]} field`, indent);
                    bloc(`//`, indent);
                    bloc(`fi.name = "${names[i]}";`, indent);
                    bloc(`fi.offset = offsetof(${fqname}, ${names[i]});`, indent);
                    // fi.getStructInfo = catena::getStructInfoFunction<types[i]>();
                    bloc(`fi.wrapGetter = [](void* dstAddr, const ParamAccessor* pa) {`, indent);
                      bloc(`auto dst = reinterpret_cast<${types[i]}*>(dstAddr);`, indent+1);
                      bloc(`pa->getValue<false, ${types[i]}>(*dst);`, indent+1);
                    bloc(`};`, indent);
                    bloc(`fi.wrapSetter = [](ParamAccessor* pa, const void* srcAddr) {`, indent);
                      bloc(`auto src = reinterpret_cast<const ${types[i]}*>(srcAddr);`, indent+1);
                      bloc(`pa->setValue<false, ${types[i]}>(*src);`, indent+1);
                    bloc(`};`, indent);
                    bloc(`t.fields.push_back(fi);`, indent);
                }
                bloc(`return t;`, bodyIndent+1)
                bloc('}', bodyIndent);
            },
            INT32: (name, desc, indent = 0) => {
                hloc(`int32_t ${name};`, indent);
            },
            FLOAT32: (name, desc, indent = 0) => {
                hloc(`float ${name};`, indent);
            },
            STRING: (name, desc, indent = 0) => {
                hloc(`std::string ${name};`, indent);
            },
            INT32_ARRAY: (name, desc, indent = 0) => {
                hloc(`std::vector<int32_t> ${name};`, indent);
            },
            FLOAT32_ARRAY: (name, desc, indent = 0) => {
                hloc(`std::vector<float> ${name};`, indent);
            },
            STRING_ARRAY: (name, desc, indent = 0) => {
                hloc(`std::vector<std::string> ${name};`, indent);
            },
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

