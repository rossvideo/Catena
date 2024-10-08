/*Copyright 2024 Ross Video Ltd
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


"use strict";

const fs = require("fs");
const { get } = require("http");
const path = require("node:path");
const Device = require("./device");
const Param = require("./param");
const LanguagePacks = require("../language");
const Constraint = require("./constraint");
const { type } = require("os");

/**
 *
 * @param {string} s
 * @returns input with the first letter capitalized
 */
function initialCap(s) {
  s;
  return s.charAt(0).toUpperCase() + s.slice(1);
}

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

/**
 * writes a line of code to a buffer
 */
class bufloc {
  constructor() {
    this.buf = "";
  }
  write(s, indent = 0) {
    this.buf += `${" ".repeat(indent * 2)}${s}\n`;
  }
  deliver(fd) {
    fs.writeSync(fd, this.buf);
  }
}

class ParamDescriptor {
  constructor(typename, name, args) {
    this.typename = typename;
    this.name = `_${name}`;
    this.args = args;
    this.subparams = [];
  }

  addSubparam(ParamDescriptor) {
    this.subparams.push(ParamDescriptor);
  }

  copySubparams(ParamDescriptor) {
    this.subparams = ParamDescriptor.subparams;
  }

  write(parent) {
    bloc(`catena::lite::ParamDescriptor ${parent}${this.name}Descriptor {${this.args}, &${parent}Descriptor, dm, false};`);
    for (let subparam of this.subparams) {
      subparam.write(`${parent}${this.name}`);
    }
  }

  writeDescriptors() {
    for (let subparam of this.subparams) {
      subparam.write(`${this.name}`);
    }
  }
}

/**
 * @class TemplateParam
 * stores enough information to use the parameter as a template, if needed.
 */
class TemplateParam {
  constructor(typename, initializer, paramDescriptor) {
    this.typename = typename;
    this.initializer = initializer;
    this.paramDescriptor = paramDescriptor;
  }

  typeName() {
    return this.typename;
  }

  initializer() {
    return this.initializer;
  }
}

/**
 * storage for the write functions, and the current indent level
 */
let hloc, bloc, ploc, postscript;
let hindent, bindent;

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
    this.postscript = new bufloc();

    let Hloc = new loc(this.header);
    hloc = Hloc.write.bind(Hloc);
    hindent = 0;
    let Bloc = new loc(this.body);
    bloc = Bloc.write.bind(Bloc);
    bindent = 0;
    let Ploc = new bufloc();
    ploc = Ploc.write.bind(Ploc);
    postscript = Ploc.deliver.bind(Ploc);

    // initialize the template param map
    this.templateParams = {};
  }

  /**
   * generate the code to instantiate the device model
   */
  device() {
    const device = new Device(this.desc);
    bloc(`catena::lite::Device dm {${device.argsToString()}};`);
    // bloc(
    //   `ParamAdder addParamToDevice = std::bind(&Device::addItem<ParamTag>, &dm, _1, _2);`
    // );
    bloc(""); // blank line
  }

  /**
   *
   * @param {string} parentOid full object id of the param's parent
   * @param {object} desc param descriptor
   * @param {boolean} isStructChild true if the param is a member of a struct
   *
   */
  params(parentOid, desc, typeNamespace, parentStructInfo, isStructChild = false, isCommand = false) {
    let isTopLevel = parentOid === "";
    let subField = isCommand ? "commands" : "params";
    if (subField in desc) {
      for (let oid in desc[subField]) {
        let structInfo = isTopLevel ? {} : parentStructInfo;
        structInfo[oid] = {};
        this.subparam(parentOid, oid, typeNamespace, desc[subField], structInfo[oid], isStructChild, isCommand);
        if (isTopLevel && structInfo[oid].params) {
          for (let type in structInfo) {
            this.defineGetStructInfo(structInfo[type]);
          }
        }
      }
    }
  }

  subparam(parentOid, oid, typeNamespace, desc, parentStructInfo, isStructChild = false, isCommand = false) {
    let p = new Param(parentOid, oid, desc);
    let args = p.argsToString();
    let type;
    let elementType;
    if (p.isTemplated()) {
      let templateParam = this.templateParam(p.templateOid());
      if (templateParam === undefined) {
        throw new Error(`No template param found for ${parentOid}/${oid}`);
      }
      if (p.isArrayType()) {
        type = p.objectType();
        elementType = templateParam.typeName();
        hloc(`using ${type} = std::vector<${elementType}>;`, hindent);
      } else {
        type = templateParam.typeName();
      }
    } else {
      type = p.objectType();
    }
    let name = p.objectName();
    let pname = p.paramName();
    let objectType = type;
    if (p.isStructType() && (!p.isTemplated() || p.isArrayType())) {
      objectType = `${typeNamespace}::${type}`;
    }
    let init = p.initializer(desc[oid]);
    if (init == "{}" && p.isTemplated()) {
      init = this.templateParam(p.templateOid()).initializer;
    }
    if (!isStructChild && p.hasValue()) {
      // only top-level params get value initializers
      bloc(`${objectType} ${name} ${init};`);
      /// @todo handle isVariant
    }
    let fqoid = `${parentOid}/${oid}`;
    let descriptor;
    if (!p.isTemplated()) {
      descriptor = new ParamDescriptor(objectType, name, args);
      this.templateParams[fqoid] = new TemplateParam(objectType, init, descriptor);
    } else {
      descriptor = new ParamDescriptor(objectType, name, args);
      descriptor.copySubparams(this.templateParams[p.templateOid()].paramDescriptor);
    }
    if (isStructChild) {
      this.templateParams[parentOid].paramDescriptor.addSubparam(descriptor);
    }

    parentStructInfo.typename = type;
    parentStructInfo.typeNamespace = typeNamespace;
    parentStructInfo.isArrayType = p.isArrayType();
    if (p.hasSubparams()) {
      hloc(`struct ${type} {`, hindent++);

      parentStructInfo.params = {};
      this.params(parentOid + `/${oid}`,  desc[oid], `${typeNamespace}::${type}`, parentStructInfo.params, true);

      hloc(`using isCatenaStruct = void;`, hindent);
      hloc(`};`, --hindent);
      if (isStructChild) {
        hloc(`${type} ${name};`, hindent);
      }
    } else if (isStructChild) {
      if (p.hasValue() ) {
        // add default value to struct member
        hloc(`${type} ${name} = ${p.initializer(desc[oid])};`, hindent);
      } else {
        hloc(`${type} ${name};`, hindent);
      }
    }

    // instantiate a ParamWithValue for top-level params, or a ParamDescriptor for struct members
    if (isCommand) {
      bloc(`catena::lite::ParamDescriptor ${descriptor.name}Descriptor {${descriptor.args}, nullptr, dm, true};`);
      descriptor.writeDescriptors();
      bloc(`catena::lite::ParamWithValue<EmptyValue> ${pname}Param {catena::lite::emptyValue, ${descriptor.name}Descriptor, dm, true};`);
    } else if (!isStructChild && p.hasValue()) {
      bloc(`catena::lite::ParamDescriptor ${descriptor.name}Descriptor {${descriptor.args}, nullptr, dm, false};`);
      descriptor.writeDescriptors();
      bloc(`catena::lite::ParamWithValue<${objectType}> ${pname}Param {${name}, ${descriptor.name}Descriptor, dm, false};`);
    }

    if (p.usesSharedConstraint()) {
      // @todo do something with the shared constraint reference
      // p.constraintRef();
    } else if (p.isConstrained() && p.hasValue()) {
      this.defineConstraint(parentOid, oid, desc);
    }
  }

  /**
   *
   * @param {string} parentOid full object id of the param's parent
   * @param {object} desc param descriptor
   * @param {boolean} isStructChild true if the param is a member of a struct
   *
   */
  commands() {
    this.params('', this.desc, this.namespace, {}, false, true);
  }

  defineGetStructInfo(structInfo) {
    ploc(`template<>`);
    ploc(`struct catena::lite::StructInfo<${structInfo.typeNamespace}::${structInfo.typename}> {`, bindent++);
    ploc(`using ${structInfo.typename} = ${structInfo.typeNamespace}::${structInfo.typename};`, bindent);
    
    const keys = Object.keys(structInfo.params);
    const lastIndex = keys.length - 1;

    let fieldInfoType = "";
    let fieldInfoInit = "";

    keys.forEach((p, index) => {
      if (structInfo.params[p].params || structInfo.params[p].isArrayType) {
        fieldInfoType += `FieldInfo<${structInfo.typename}::${structInfo.params[p].typename}, ${structInfo.typename}>`;
      } else {
        fieldInfoType += `FieldInfo<${structInfo.params[p].typename}, ${structInfo.typename}>`;
      }
      fieldInfoInit += `{"${p}", &${structInfo.typename}::${p}}`;
      if (index !== lastIndex) {
        fieldInfoType += `, `;
        fieldInfoInit += `, `;
      } 
    });
    ploc(`using Type = std::tuple<${fieldInfoType}>;`, bindent);
    ploc(`static constexpr Type fields = {${fieldInfoInit}};`, bindent);
    ploc(`};`, --bindent);

    // define getStructInfo for subparams structs
    for (let p in structInfo.params) {
      if (structInfo.params[p].params) {
        this.defineGetStructInfo(structInfo.params[p]);
      }
    }
  }

  /**
   * 
   * shared constraints defined at the top level 
   * @param {object} desc constraint descriptor 
   * 
   */
  constraints(desc) {
    if ("constraints" in desc) {
      for (let oid in desc.constraints) {
        this.defineConstraint("constraints", oid, desc.constraints);
      }
    }
  }

  /**
   * 
   * shared constraints defined at the top level 
   * @param {string} parentOid parent object id
   * @param {string} oid object id
   * @param {object} desc constraint descriptor 
   * 
   */
  defineConstraint(parentOid, oid, desc) {
    let c = new Constraint(parentOid, oid, desc);
    let args = c.argsToString();
    let constraintType = c.objectType();
    let cname = c.constraintName(); 
    if (c.isShared()) {
      bloc(`catena::lite::${constraintType} ${cname}Constraint {${args}};`);
    } else {
      bloc(`catena::lite::${constraintType} ${cname}ParamConstraint {${args}};`);
    }
  }

  /**
   * 
   * @param {string} fully qualified object id
   * @returns undefined or the template param for the given oid
   */
  templateParam(oid) {
    return this.templateParams[oid];
  }

  /**
   * log the template parameters to the console
   */
  logTemplateParams() {
    console.log(`template params: ${JSON.stringify(this.templateParams, null, 2)}`);
  }

  languagePacks() {
    let languagePacks = new LanguagePacks(this.desc);
    let packs = languagePacks.getLanguagePacks();    
    bloc (`using catena::lite::LanguagePack;`);
    for (let pack in packs) {
      let lang = packs[pack];
      bloc(`LanguagePack ${pack} {`);
      bloc(`"${lang.name}",`,1);
      let keyWordPairs = Object.keys(lang.words);
      bloc(`{`,1);
      bloc(keyWordPairs.map((key) => {return `{ "${key}", "${lang.words[key]}" }`}).join(",\n    "),2);
      bloc(`},`,1);
      bloc(`dm`,1);
      bloc(`};`);
    }
  }

  /**
   * generate header and body files to represent the device model
   */
  generate() {
    this.init();
    this.device();
    this.languagePacks();
    this.params('', this.desc, this.namespace);
    this.commands('', this.desc, this.namespace);
    this.constraints(this.desc);
    this.finish();
  }

  init() {
    const warning = `// This file was auto-generated. Do not modify by hand.`;
    hloc(`#pragma once`);
    hloc(warning);
    hloc(`#include <Device.h>`);
    hloc(`#include <StructInfo.h>`);
    hloc(`extern catena::lite::Device dm;`);
    hloc(`namespace ${this.namespace} {`);

    bloc(warning);
    bloc(`#include "${this.headerFilename}"`);
    bloc(`using namespace ${this.namespace};`);
    bloc(`#include <ParamDescriptor.h>`);
    bloc(`#include <ParamWithValue.h>`);
    bloc(`#include <LanguagePack.h>`);
    bloc(`#include <Device.h>`);
    bloc(`#include <RangeConstraint.h>`);
    bloc(`#include <PicklistConstraint.h>`);
    bloc(`#include <NamedChoiceConstraint.h>`);
    bloc(`#include <Enums.h>`);
    bloc(`#include <StructInfo.h>`);
    bloc(`#include <string>`);
    bloc(`#include <vector>`);
    bloc(`#include <functional>`);
    bloc(`using catena::Device_DetailLevel;`);
    bloc(`using DetailLevel = catena::common::DetailLevel;`);
    bloc(`using catena::common::Scopes_e;`);
    bloc(`using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;`);
    bloc(`using catena::lite::FieldInfo;`);
    bloc(`using catena::lite::ParamDescriptor;`);
    bloc(`using catena::lite::ParamWithValue;`);
    bloc(`using catena::lite::Device;`);
    bloc(`using catena::lite::RangeConstraint;`);
    bloc(`using catena::lite::PicklistConstraint;`);
    bloc(`using catena::lite::NamedChoiceConstraint;`);
    bloc(`using catena::common::IParam;`);
    bloc(`using catena::lite::EmptyValue;`);
    bloc(`using std::placeholders::_1;`);
    bloc(`using std::placeholders::_2;`);
    bloc(`using catena::common::ParamTag;`);
    bloc(`using ParamAdder = catena::common::AddItem<ParamTag>;`);
  }

  finish = () => {
    hloc(`} // namespace ${this.namespace}`);
    postscript(this.header);

  };
}

module.exports = CppGen;
