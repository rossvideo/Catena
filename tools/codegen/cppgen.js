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
const { get } = require('http');

const kCppTypes = {
    "INT32": "int32_t",
    "FLOAT32": "float",
    "STRING": "std::string",
}

function initialCap(s) {
    return s.charAt(0).toUpperCase() + s.slice(1);
}


const getFieldInit = {
    "INT32": (value) => `${value.int32_value}`,
    "FLOAT32": (value) => `${value.float32_value}`,
    "STRING": (value) => `"${value.string_value}"`
};

function structInit(names, srctypes, desc) {
    let inits = [];
    let fields = desc.value.struct_value.fields;
    if ("value" in desc) {
        for (let i = 0; i < names.length; ++i) {
            if (names[i] in fields) {
                let field = fields[names[i]];
                let srctype = srctypes[i];
                if (srctype in kCppTypes) {
                    inits.push(getFieldInit[srctype](field.value));
                } else {
                    inits.push('{}');
                }
            } else {
                inits.push('{}');
            }
        }
    } else {
        inits.push('{}');
    }
    return `{${inits.join(', ')}}`;
}

class CppGen {
    constructor(hloc, bloc, namespace) {
        this.hloc = hloc; // write a line of code to the header file
        this.bloc = bloc; // write a line of code to the body file
        this.arrayInitializer = (name, desc, arr, quote = '', indent = 0) => {
            let initializer = '{}';
            if ("value" in desc) {
                initializer = '{';
                for (let i = 0; i < arr.length; ++i) {
                    initializer += `${quote}${arr[i]}${quote}`;
                    if (i < arr.length - 1) {
                        initializer += ',';
                    }
                }
                initializer += '}';
            }
            return initializer;
        };
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
                let srctypes = [];
                let defaults = [];
                // gather information about the struct
                for (p in params) {
                    const type = params[p].type;
                    names.push(p);
                    srctypes.push(type);
                    if (type in kCppTypes) {
                        let cppType = kCppTypes[type];
                        types.push(cppType);
                    } else if (type === "STRUCT") {
                        let userDefinedType = p.charAt(0).toUpperCase() + p.slice(1);
                        types.push(userDefinedType);
                        this.convertors.STRUCT(userDefinedType, params[p], indent + 1);
                    }
                    if ("value" in params[p]) {
                        defaults.push(`= ${getFieldInit[type](params[p].value)}`);
                    } else {
                        defaults.push('{}');
                    }
                    ++n;
                }
                
                // write the struct to the header file
                for (let i = 0; i < n; ++i) {
                    hloc(`${types[i]} ${names[i]} ${defaults[i]};`, indent+1);
                }

                // declare the getStructInfo method and typelist in the header file
                hloc(`static const catena::lite::StructInfo& getStructInfo();`, indent+1)
                // hloc(`using typelist = catena::meta::TypeList<${types.join(', ')}>;`, indent+1)
                hloc(`};`, indent);

                // instantiate the struct in the body file 
                let bodyIndent = 0;
                bloc(`${fqname} ${name} ${structInit(names, srctypes, desc)};`, bodyIndent);
                bloc(`catena::lite::Param<${fqname}> ${name}Param(${name},"${name}",dm);`, bodyIndent)
                bloc(`const StructInfo& ${fqname}::getStructInfo() {`, bodyIndent);
                bloc(`static StructInfo t;`, bodyIndent+1);
                bloc(`if (t.name.length()) return t;`, bodyIndent+1);
                bloc(`t.name = "${classname}";`, bodyIndent+1);
                bloc(`FieldInfo fi;`, bodyIndent+1);
                for (let i = 0; i < n; ++i) {
                    let indent = bodyIndent+2;
                    bloc(`// register info for the ${names[i]} field`, indent);
                    bloc(`fi.name = "${names[i]}";`, indent);
                    bloc(`fi.offset = offsetof(${fqname}, ${names[i]});`, indent);
                    bloc(`fi.toProto = catena::lite::toProto<${types[i]}>;`, indent);
                    bloc(`t.fields.push_back(fi);`, indent);
                }
                bloc(`return t;`, bodyIndent+1)
                bloc('}', bodyIndent);

                // instantiate the serialize specialization
                bloc(`template<>`, indent);
                bloc(`void catena::lite::Param<${fqname}>::toProto(catena::Value& value) const {`, indent);
                bloc(`catena::lite::toProto<${fqname}>(value, &value_.get());`, indent+1);
                bloc('}', indent);
            },
            "STRING": (name, desc, indent = 0) => {
                let initializer = '{}';
                if ("value" in desc) {
                    initializer = `{"${desc.value.string_value}"}`;
                }
                bloc(`std::string ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<std::string> ${name}Param(${name},"${name}",dm);`, indent);
            },
            "INT32": (name, desc, indent = 0) => {
                let initializer = '{}';
                if ("value" in desc) {
                    initializer = `{${desc.value.int32_value}}`;
                }
                bloc(`int32_t ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<int32_t> ${name}Param(${name},"${name}",dm);`, indent);
            },
            "FLOAT32": (name, desc, indent = 0) => {
                let initializer = '{}';
                if ("value" in desc) {
                    initializer = `{${desc.value.float32_value}}`;
                }
                bloc(`float ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<float> ${name}Param(${name},"${name}",dm);`, indent);
            },
            "STRING_ARRAY": (name, desc, indent = 0) => {
                let initializer = this.arrayInitializer(name, desc, desc.value.string_array_values.strings, '"', indent);
                bloc(`std::vector<std::string> ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<std::vector<std::string>> ${name}Param(${name},"${name}",dm);`, indent);
            },
            "INT32_ARRAY": (name, desc, indent = 0) => {
                let initializer = this.arrayInitializer(name, desc, desc.value.int32_array_values.ints, '', indent);
                bloc(`std::vector<std::int32_t> ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<std::vector<std::int32_t>> ${name}Param(${name},"${name}",dm);`, indent);
            },
            "FLOAT32_ARRAY": (name, desc, indent = 0) => {
                let initializer = this.arrayInitializer(name, desc, desc.value.float32_array_values.floats, '', indent);
                bloc(`std::vector<float> ${name}${initializer};`, indent);
                bloc(`catena::lite::Param<std::vector<float>> ${name}Param(${name},"${name}",dm);`, indent);
            }
        };
        this.init = (headerFilename) => {
            const warning = `// This file was auto-generated. Do not modify by hand.`;
            hloc(`#pragma once`);
            hloc(warning);
            hloc(`#include <lite/include/Device.h>`);
            hloc(`#include <lite/include/StructInfo.h>`);
            hloc(`extern catena::lite::Device dm;`);
            hloc(`namespace ${namespace} {`)
            bloc(warning);
            bloc(`#include "${headerFilename}"`);
            bloc(`#include <lite/include/IParam.h>`);
            bloc(`#include <lite/include/Param.h>`);
            bloc(`#include <lite/include/Device.h>`);
            bloc(`catena::lite::Device dm{};`)
            bloc(`using catena::lite::StructInfo;`);
            bloc(`using catena::lite::FieldInfo;`);
        },
        this.finish = () => {
            hloc(`} // namespace ${namespace}`);
        }
    }
    
    convert (oid, desc) {
        if (desc.type in this.convertors) {
            return this.convertors[desc.type](oid, desc);
        } else if ("template_oid" in desc) {
            // code to handle templated params here
        } else {
            console.log(`No convertor found for ${oid} of type ${desc.type}`);
        }
    }
};

module.exports = CppGen;

