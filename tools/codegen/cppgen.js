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
const LanguagePacks = require("./language");
const Constraint = require("./constraint");
const { type } = require("os");

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
 * @class TemplateParam
 * stores enough information to use the parameter as a template, if needed.
 */
class TemplateParam {
  constructor(typename, initializer) {
    this.typename = typename;
    this.initializer = initializer;
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
let hloc, bloc;
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

    let Hloc = new loc(this.header);
    hloc = Hloc.write.bind(Hloc);
    hindent = 0;
    let Bloc = new loc(this.body);
    bloc = Bloc.write.bind(Bloc);
    bindent = 0;

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
  params(parentOid, desc, typeNamespace, parentStructInfo, isStructChild = false) {
    let isTopLevel = parentOid === "";
    if ("params" in desc) {
      for (let oid in desc.params) {
        let structInfo = isTopLevel ? {} : parentStructInfo;
        structInfo[oid] = {};
        this.subparam(parentOid, oid, typeNamespace, desc.params, structInfo[oid], isStructChild);
        if (isTopLevel && structInfo[oid].params) {
          for (let type in structInfo) {
            this.defineGetStructInfo(structInfo[type]);
          }
        }
      }
    }
  }

  subparam(parentOid, oid, typeNamespace, desc, parentStructInfo, isStructChild = false) {
    let p = new Param(parentOid, oid, desc);
    let args = p.argsToString();
    let type;
    if (p.isTemplated()) {
      let templateParam = this.templateParam(p.templateOid());
      if (templateParam === undefined) {
        throw new Error(`No template param found for ${parentOid}/${oid}`);
      }
      type = templateParam.typeName();
    } else {
      type = p.objectType();
    }
    let name = p.objectName();
    let pname = p.paramName();
    let objectType = type;
    if (p.isStructType() && !p.isTemplated()) {
      objectType = `${typeNamespace}::${type}`;
    }
    let init = p.initializer(desc[oid]);
    if (init == "{}" && p.isTemplated()) {
      init = this.templateParam(p.templateOid()).initializer;
    }
    if (!isStructChild) {
      // only top-level params get value objects
      bloc(`${objectType} ${name} ${init};`);
      /// @todo handle isVariant
    }
    let fqoid = `${parentOid}/${oid}`;
    if (~p.isTemplated()) {
      this.templateParams[fqoid] = new TemplateParam(objectType, init);
    }

    // instantiate a ParamWithValue for top-level params, or a ParamDescriptor for struct members
    if (!isStructChild) {
      bloc(`catena::lite::ParamWithValue<${objectType}> ${pname}Param {${args}, ${name}};`);
    } else {
      bloc(`catena::lite::ParamDescriptor<${objectType}> ${pname}Param {${args}};`);
    }

    parentStructInfo.typename = type;
    parentStructInfo.typeNamespace = typeNamespace;
    if (p.hasSubparams()) {
      hloc(`struct ${type} {`, hindent++);

      parentStructInfo.params = {};
      this.params(parentOid + `/${oid}`,  desc[oid], `${typeNamespace}::${type}`, parentStructInfo.params, true);

      hloc(`static const catena::lite::StructInfo& getStructInfo();`, hindent);
      hloc(`};`, --hindent);
      if (isStructChild) {
        hloc(`${type} ${name};`, hindent);
      }
    } else if (isStructChild) {
      hloc(`${type} ${name};`, hindent);
    }

    if (p.usesSharedConstraint()) {
      // @todo do something with the shared constraint reference
      // p.constraintRef();
    } else if (p.isConstrained()) {
      this.defineConstraint(parentOid, oid, desc);
    }
  }

  defineGetStructInfo(structInfo) {
    bloc(`const StructInfo& ${structInfo.typeNamespace}::${structInfo.typename}::getStructInfo() {`, bindent++);
    bloc(`static StructInfo si {`, bindent++);
    bloc(`"${structInfo.typename}",`, bindent);
    bloc(`{`, bindent++);
    
    const keys = Object.keys(structInfo.params);
    const lastIndex = keys.length - 1;

    keys.forEach((p, index) => {
      if (index === lastIndex) {
        bloc(`{ "${p}", offsetof(${structInfo.typename}, ${p})}`, bindent);
      } else {
        bloc(`{ "${p}", offsetof(${structInfo.typename}, ${p})},`, bindent);
      }
    });

    bloc(`}`, --bindent);
    bloc(`};`, --bindent);
    bloc(`return si;`, bindent);
    bloc(`}`, --bindent);

    // defince getStructInfo for subparams structs
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
    bloc(`using catena::lite::StructInfo;`);
    bloc(`using catena::lite::FieldInfo;`);
    bloc(`using catena::lite::ParamDescriptor;`);
    bloc(`using catena::lite::ParamWithValue;`);
    bloc(`using catena::lite::Device;`);
    bloc(`using catena::lite::RangeConstraint;`);
    bloc(`using catena::lite::PicklistConstraint;`);
    bloc(`using catena::lite::NamedChoiceConstraint;`);
    bloc(`using catena::common::IParam;`);
    bloc(`using std::placeholders::_1;`);
    bloc(`using std::placeholders::_2;`);
    bloc(`using catena::common::ParamTag;`);
    bloc(`using ParamAdder = catena::common::AddItem<ParamTag>;`);
  }

  finish = () => {
    hloc(`} // namespace ${this.namespace}`);
  };
}

module.exports = CppGen;
