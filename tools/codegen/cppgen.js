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

"use strict";

const fs = require("fs");
const { get } = require("http");
const path = require("node:path");
const Device = require("./device");
const Param = require("./param");

/**
 * writes a line of code to the file descriptor constructed
 */
class loc {
  constructor(fd) {
    this.fd = fd;
  }
  /**
   *
   * @param {string} s the line of code to write
   * @param {number} indent how many 2-space indents to prepend
   */
  write(s, indent = 0) {
    fs.writeSync(this.fd, `${" ".repeat(indent * 2)}${s}\n`);
  }
}

let hloc, bloc;

/**
 * C++ code generator
 */
class CppGen {
  /**
   *
   * @param {string} pathname to the device model's top-level json file
   * @param {string} output folder for generated code
   * @param {Validator} validator used to validate that descriptors match catena schemas
   * @param {object} desc json descriptor
   */
  constructor(pathname, output, validator, desc) {
    this.pathname = pathname;
    this.validator = validator;
    this.desc = desc;
    // extract schema name from input filename
    const info = path.parse(pathname).name.split(".");
    const schemaName = info[0];
    if (schemaName !== "device") {
      throw new Error(`File must be a device model, not ${schemaName}`);
    }
    this.namespace = info[1];
    const baseFilename = path.basename(pathname);
    this.headerFilename = `${baseFilename}.h`;
    this.header = fs.openSync(path.join(output, this.headerFilename), "w");
    this.body = fs.openSync(path.join(output, `${baseFilename}.cpp`), "w");

    let Hloc = new loc(this.header);
    hloc = Hloc.write.bind(Hloc);
    let Bloc = new loc(this.body);
    bloc = Bloc.write.bind(Bloc);
  }

  //     arrayInitializer (name, desc, arr, quote = '', indent = 0)  {
  //         let initializer = '{}';
  //         if ("value" in desc) {
  //             initializer = '{';
  //             for (let i = 0; i < arr.length; ++i) {
  //                 initializer += `${quote}${arr[i]}${quote}`;
  //                 if (i < arr.length - 1) {
  //                     initializer += ',';
  //                 }
  //             }
  //             initializer += '}';
  //         }
  //         return initializer;
  //     };

  //     constraints = {
  //         "FLOAT_RANGE": (name, desc, indent = 0) => {
  //            // place holder for now
  //         }
  //     }

  //    other_items = (desc, template) => {
  //         let ans = '';

  //         // add oid_aliases if they exist
  //         let aliases_init = '{';
  //         if (desc.oid_aliases !== undefined) {
  //             const aliases = desc.oid_aliases.map(alias => {return quoted(alias);});
  //             aliases_init += `${aliases.join(',')}`;
  //         }
  //         aliases_init += '}';
  //         ans += `${aliases_init},`;

  //         // add the name if it exists
  //         let name_init = '{';
  //         if (desc.name !== undefined) {
  //             const display_strings = desc.name.display_strings;
  //             const n = Object.keys(display_strings).length;
  //             let i = 0;
  //             for (let lang in display_strings) {
  //                 name_init += `{"${lang}", ${quoted(display_strings[lang])}}`;
  //                 if (i++ < n-1) {
  //                     name_init += ',';
  //                 }
  //             }
  //         }
  //         name_init += '}';
  //         ans += `${name_init},`;

  //         // add the widget if it exists
  //         let widget_init = '{';
  //         if (desc.widget !== undefined) {
  //             const widget = desc.widget;
  //             widget_init += `${quoted(widget)}`;
  //         } else if (template !== undefined && template.widget !== undefined) {
  //             widget_init += `${quoted(template.widget)}`;
  //         }
  //         widget_init += '}';
  //         ans += `${widget_init}`;

  //         // add the read_only flag
  //         if (desc.read_only !== undefined && desc.read_only) {
  //             ans += `,true`;
  //         } else {
  //             ans += `,false`;
  //         }

  //         return ans;
  //     }

  //     params {
  //         "STRUCT": (name, desc, template, scope = undefined, indent = 0) => {
  //             let params;
  //             let classname;
  //             if (template === undefined) {
  //                 params = desc.params;
  //                 classname = initialCap(name);
  //             } else {
  //                 params = template.params;
  //                 classname = initialCap(desc.template_oid.replace(/^\//, ''));
  //             }
  //             if (scope === undefined) {
  //                 scope = namespace;
  //             }
  //             const fqname = `${scope}::${classname}`;
  //             let n = 0;
  //             let types = [];
  //             let names = [];
  //             let srctypes = [];
  //             let defaults = [];
  //             // gather information about the struct
  //             for (let p in params) {
  //                 const type = params[p].type;
  //                 names.push(p);
  //                 srctypes.push(type);
  //                 if (type in kCppTypes) {
  //                     let cppType = kCppTypes[type];
  //                     types.push(cppType);
  //                 } else if (type === "STRUCT") {
  //                     let userDefinedType = p.charAt(0).toUpperCase() + p.slice(1);
  //                     types.push(userDefinedType);
  //                     this.params.STRUCT(userDefinedType, params[p], fqname, indent + 1);
  //                 }
  //                 if ("value" in params[p]) {
  //                     defaults.push(`= ${getFieldInit[type](params[p].value)}`);
  //                 } else {
  //                     defaults.push('{}');
  //                 }
  //                 // if ("value" in params[p]) {
  //                 //     defaults.push(`= ${getFieldInit[type](params[p].value)}`);
  //                 // } else {
  //                 //     defaults.push('{}');
  //                 // }
  //                 ++n;
  //             }

  //             if (template === undefined) {
  //                 // write the struct to the header file
  //                 hloc(`struct ${classname} {`, indent);
  //                 for (let i = 0; i < n; ++i) {
  //                     hloc(`${types[i]} ${names[i]} ${defaults[i]};`, indent+1);
  //                 }

  //                 // declare the getStructInfo method and typelist in the header file
  //                 hloc(`static const catena::lite::StructInfo& getStructInfo();`, indent+1)
  //                 hloc(`};`, indent);
  //             }

  //             // instantiate the struct in the body file
  //             let bodyIndent = 0;

  //             if (template !== undefined) {
  //                 bloc(`${fqname} ${name} ${structInit(names, srctypes, desc)};`, bodyIndent);
  //                 bloc(`catena::lite::Param<${fqname}> ${name}Param(catena::ParamType::STRUCT,${name},${this.other_items(desc)},"/${name}",dm);`, bodyIndent)
  //                 // Support functions for user defined types have already been generated
  //                 return;
  //             } else {
  //                 bloc(`${fqname} ${name} ${structInit(names, srctypes, desc)};`, bodyIndent);
  //                 bloc(`catena::lite::Param<${fqname}> ${name}Param(catena::ParamType::STRUCT,${name},${this.other_items(desc)},"/${name}",dm);`, bodyIndent)
  //             }
  //             bloc(`const StructInfo& ${fqname}::getStructInfo() {`, bodyIndent);
  //             bloc(`static StructInfo t {`, bodyIndent+1);
  //             bloc(`"${name}", {`, bodyIndent+2);
  //             for (let i = 0; i < n; ++i) {
  //                 let indent = bodyIndent+3;
  //                 bloc(`{ "${names[i]}", offsetof(${fqname}, ${names[i]}), catena::lite::toProto<${types[i]}>, catena::lite::fromProto<${types[i]}> }${i<n-1?',':''}`, indent);
  //             }
  //             bloc(`}`, bodyIndent+2);
  //             bloc(`};`, bodyIndent+1);
  //             bloc(`return t;`, bodyIndent+1)
  //             bloc('}', bodyIndent);

  //             // instantiate the serialize specialization
  //             bloc(`template<>`, indent);
  //             bloc(`void catena::lite::Param<${fqname}>::toProto(catena::Value& value) const {`, indent);
  //             bloc(`catena::lite::toProto<${fqname}>(value, &value_.get());`, indent+1);
  //             bloc('}', indent);

  //             // instantiate the deserialize specialization
  //             bloc(`template<>`, indent);
  //             bloc(`void catena::lite::Param<${fqname}>::fromProto(const catena::Value& value) {`, indent);
  //             bloc(`catena::lite::fromProto<${fqname}>(&value_.get(), value);`, indent+1);
  //             bloc('}', indent);
  //         },
  //         "STRING": (name, desc, template, indent = 0) => {
  //             let initializer = '{}';
  //             if ("value" in desc) {
  //                 initializer = `{"${desc.value.string_value}"}`;
  //             }
  //             bloc(`std::string ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<std::string> ${name}Param(catena::ParamType::STRING,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);
  //         },
  //         "INT32": (name, desc, template, indent = 0) => {
  //             let initializer = '{}';
  //             if ("value" in desc) {
  //                 initializer = `{${desc.value.int32_value}}`;
  //             }
  //             bloc(`int32_t ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<int32_t> ${name}Param(catena::ParamType::INT32,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);

  //         },
  //         "FLOAT32": (name, desc, template, indent = 0) => {
  //             let initializer = '{}';
  //             if ("value" in desc) {
  //                 initializer = `{${desc.value.float32_value}}`;
  //             }
  //             bloc(`float ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<float> ${name}Param(catena::ParamType::FLOAT32,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);
  //         },
  //         "STRING_ARRAY": (name, desc, template, indent = 0) => {
  //             let initializer = this.arrayInitializer(name, desc, desc.value.string_array_values.strings, '"', indent);
  //             bloc(`std::vector<std::string> ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<std::vector<std::string>> ${name}Param(catena::ParamType::STRING_ARRAY,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);
  //         },
  //         "INT32_ARRAY": (name, desc, template, indent = 0) => {
  //             let initializer = this.arrayInitializer(name, desc, desc.value.int32_array_values.ints, '', indent);
  //             bloc(`std::vector<std::int32_t> ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<std::vector<std::int32_t>> ${name}Param(catena::ParamType::INT32_ARRAY,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);
  //         },
  //         "FLOAT32_ARRAY": (name, desc, template, indent = 0) => {
  //             let initializer = this.arrayInitializer(name, desc, desc.value.float32_array_values.floats, '', indent);
  //             bloc(`std::vector<float> ${name}${initializer};`, indent);
  //             bloc(`catena::lite::Param<std::vector<float>> ${name}Param(catena::ParamType::FLOAT32_ARRAY,${name},${this.other_items(desc,template)},"/${name}",dm);`, indent);
  //         }
  //     };

  /**
   * generate the code to instantiate the device model
   */
  device() {
    const device = new Device(this.desc);
    bloc(`catena::lite::Device dm {${device.argsToString()}};`);
    bloc(
      `ParamAdder addParamToDevice = std::bind(&Device::addItem<ParamTag>, &dm, _1, _2);`
    );
    bloc(""); // blank line
  }

  /**
   *
   * @param {string} parentOid full object id of the param's parent
   * @param {object} desc param descriptor
   * @param {boolean} isStructChild true if the param is a member of a struct
   *
   */
  params(parentOid, desc, parentStructInfo, isStructChild = false) {
    if ("params" in desc) {
      for (let oid in desc.params) {
        let structInfo = (parentOid === '') ? {} : parentStructInfo;
        structInfo[oid] = {};
        this.subparam(
          parentOid,
          oid,
          desc.params,
          structInfo[oid],
          isStructChild
        );
        if (parentStructInfo === undefined) {
          console.log(`rollup: ${JSON.stringify(structInfo, null, 2)}`);
        }
      }
    }
  }

  subparam(parentOid, oid, desc, parentStructInfo, isStructChild = false) {
    let p = new Param(parentOid, oid, desc);
    let args = p.argsToString();
    let type = p.objectType();
    let name = p.objectName();
    let pname = p.paramName();
    if (!isStructChild) {
      // only top-level params get value objects
      let init = p.initializer(desc[oid]);
      bloc(`${type} ${name} ${init};`);
    }

    // instantiate the ParamDescriptor
    bloc(`catena::lite::ParamDescriptor<${type}> ${pname}Param {${args}};`);

    if (!isStructChild) {
      // only top-level params get ParamWithValue objects
      bloc(
        `catena::lite::ParamWithValue<${type}> ${pname}ParamWithValue {${pname}Param, ${name}};`
      );
    }

    parentStructInfo.typename = type;
    if (p.hasSubparams()) {
      parentStructInfo.params = {};
      this.params(
        parentOid + `/${oid}`,
        desc[oid],
        parentStructInfo.params,
        true
      );
    }
  }

  /**
   * generate header and body files to represent the device model
   */
  generate() {
    this.init();
    this.device();
    this.params("", this.desc);
    this.finish();
  }

  init() {
    const warning = `// This file was auto-generated. Do not modify by hand.`;
    hloc(`#pragma once`);
    hloc(warning);
    hloc(`#include <lite/include/Device.h>`);
    hloc(`#include <lite/include/StructInfo.h>`);
    hloc(`extern catena::lite::Device dm;`);
    hloc(`namespace ${this.namespace} {`);

    bloc(warning);
    bloc(`#include "${this.headerFilename}"`);
    bloc(`#include <lite/include/ParamDescriptor.h>`);
    bloc(`#include <lite/include/ParamWithValue.h>`);
    bloc(`#include <lite/include/Device.h>`);
    bloc(`#include <common/include/Enums.h>`);
    bloc(`#include <lite/include/StructInfo.h>`);
    bloc(`#include <string>`);
    bloc(`#include <vector>`);
    bloc(`#include <functional>`);
    bloc(`using catena::Device_DetailLevel;`);
    bloc(`using DetailLevel = catena::common::DetailLevel;`);
    bloc(`using catena::common::Scopes_e;`);
    bloc(`using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;`);
    bloc(`using catena::lite::StructInfo;`);
    bloc(`using catena::lite::FieldInfo;`);
    bloc(`using catena::lite::ParamDescriptor;`);
    bloc(`using catena::lite::ParamWithValue;`);
    bloc(`using catena::lite::Device;`);
    bloc(`using catena::lite::IParam;`);
    bloc(`using std::placeholders::_1;`);
    bloc(`using std::placeholders::_2;`);
    bloc(`using catena::common::ParamTag;`);
    bloc(`using ParamAdder = catena::common::AddItem<ParamTag>;`);
  }

  finish = () => {
    hloc(`} // namespace ${this.namespace}`);
  };

  // param (oid, desc, device) {
  //     if (!desc.type in this.params) {
  //         throw new Error(`No convertor found for ${oid} of type ${desc.type}`);
  //     }
  //     let template_param = undefined;
  //     if ("template_oid" in desc) {
  //         let template_oid = desc.template_oid.replace(/^\//, '');
  //         template_param = device.params[template_oid];
  //         if (template_param === undefined) {
  //             throw new Error(`Could not find template ${template_oid} for ${oid}`);
  //         }
  //         if (template_param.type !== desc.type) {
  //             throw new Error(`Template ${template_oid} type ${template_param.type} does not match ${oid} type ${desc.type}`);
  //         }
  //         if (desc.params !== undefined) {
  //             throw new Error(`Param ${oid} is based off a template so it can't have params`);
  //         }
  //     }
  //     return this.params[desc.type](oid, desc, template_param);
  // }
}

module.exports = CppGen;
