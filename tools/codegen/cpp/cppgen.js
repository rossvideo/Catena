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
 * writes a line of code to the file descriptor constructed
 * @param {File} fd file descriptor
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
 * @param {File} fd file descriptor
 */
class bufloc {
  constructor(fd) {
    this.buf = "";
    this.fd = fd;
  }

  /**
   * 
   * @param {string} s the line of code to write 
   * @param {number} indent how many 2-space indents to prepend 
   */
  write(s, indent = 0) {
    this.buf += `${" ".repeat(indent * 2)}${s}\n`;
  }

  /**
   * write the buffer to the file descriptor
   */
  deliver() {
    fs.writeSync(this.fd, this.buf);
  }
}

/**
 * storage for the write functions, and the current indent level
 */
let hloc, bloc, ploc, cloc, postscript, coda;
let hindent, pindent;

/**
 * @class CppGen
 * C++ code generator
 */
class CppGen {
  /**
   *
   * @param {DeviceModel} deviceModel to the device model's top-level json file
   * @param {string} outputDir folder for generated code
   */
  constructor(deviceModel, outputDir) {
    this.headerFilename = `${deviceModel.baseFilename}.h`;
    let header = fs.openSync(path.join(outputDir, `${this.headerFilename}`), "w");
    let body = fs.openSync(path.join(outputDir, `${deviceModel.baseFilename}.cpp`), "w");

    let Hloc = new loc(header);
    hloc = Hloc.write.bind(Hloc);
    hindent = 0;
    let Bloc = new loc(body);
    bloc = Bloc.write.bind(Bloc);
    pindent = 0;
    let Ploc = new bufloc(header);
    ploc = Ploc.write.bind(Ploc);
    postscript = Ploc.deliver.bind(Ploc);
    let Cloc = new bufloc(body);
    cloc = Cloc.write.bind(Cloc);
    coda = Cloc.deliver.bind(Cloc);

    this.device = new Device(deviceModel);
  }

  /**
   * generate header and body files to represent the device model
   */
  generate() {
    this.init();
    this.deviceInit();
    this.languagePacks();
    this.menu();
    this.constraints();
    this.params();
    this.commands();
    this.finish();
  }

  /**
   * generate the header and body file preamble
   */
  init() {
    const warning = `// This file was auto-generated. Do not modify by hand.`;
    hloc(`#pragma once`);
    hloc(warning);
    hloc(`#include <Device.h>`);
    hloc(`#include <StructInfo.h>`);
    hloc(`extern catena::common::Device dm;`);
    hloc(`namespace ${this.device.namespace} {`);

    bloc(warning);
    bloc(`#include "${this.headerFilename}"`);
    bloc(`using namespace ${this.device.namespace};`);
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
    bloc(`#include <Menu.h>`);
    bloc(`#include <MenuGroup.h>`);
    bloc(`using catena::Device_DetailLevel;`);
    bloc(`using DetailLevel = catena::common::DetailLevel;`);
    bloc(`using catena::common::Scopes_e;`);
    bloc(`using Scope = typename catena::patterns::EnumDecorator<Scopes_e>;`);
    bloc(`using catena::common::FieldInfo;`);
    bloc(`using catena::common::ParamDescriptor;`);
    bloc(`using catena::common::ParamWithValue;`);
    bloc(`using catena::common::Device;`);
    bloc(`using catena::common::RangeConstraint;`);
    bloc(`using catena::common::PicklistConstraint;`);
    bloc(`using catena::common::NamedChoiceConstraint;`);
    bloc(`using catena::common::IParam;`);
    bloc(`using catena::common::EmptyValue;`);
    bloc(`using std::placeholders::_1;`);
    bloc(`using std::placeholders::_2;`);
    bloc(`using catena::common::ParamTag;`);
    bloc(`using ParamAdder = catena::common::AddItem<ParamTag>;`);
  }

  /**
   * generate the code to instantiate the device model
   */
  deviceInit() {
    bloc(`catena::common::Device dm {${this.device.argsToString()}};`);
    bloc(""); // blank line
  }

  /**
   * generate the code to instantiate the language packs
   */
  languagePacks() {
    let languagePacks = new LanguagePacks(this.device.desc);
    let packs = languagePacks.getLanguagePacks();    
    bloc (`using catena::common::LanguagePack;`);
    for (let pack in packs) {
      let lang = packs[pack];
      let keyWordPairs = Object.keys(lang.words);
      bloc(`LanguagePack ${pack} {`);
      bloc(`"${lang.name}",`, 1);
      bloc(`{`, 1);
      bloc(keyWordPairs.map((key) => { return `{ "${key}", "${lang.words[key]}" }` }).join(",\n    "), 2);
      bloc(`},`, 1);
      bloc(`dm`, 1);
      bloc(`};`);
    }
  }

  /**
   * generate the code to instantiate the menu groups and menus
   */
  menu() {
    bloc(`using catena::common::Menu;`);
    bloc(`using catena::common::MenuGroup;`);

    let menuGroups = this.device.desc.menu_groups;
    for (let group in menuGroups) {
      let groupName = menuGroups[group].name.display_strings;
      let groupNamePairs = Object.keys(groupName);
      bloc(`MenuGroup _${group}Group {\n  "${group}", `);
      bloc(`  {\n    ${groupNamePairs.map((key) => { return `{ "${key}", "${groupName[key]}" }` }).join(",\n    ")}`);
      bloc(`  },\n  dm\n};`);
      
      let menus = menuGroups[group].menus;
      for (let menu in menus) {
        let paramOids = menus[menu].param_oids.map(oid => `"${oid}"`).join(", ");
        let menuName = menus[menu].name.display_strings;
        let menuNamePairs = Object.keys(menuName);
        bloc(`Menu _${group}Group_${menu}Menu {\n  {    `);
        bloc(`    ${menuNamePairs.map((key) => { return `{ "${key}", "${menuName[key]}" }` }).join(",\n    ")}\n  },`);
        bloc(`  false, false, `);
        bloc(`  {${paramOids}}, `);
        bloc(`  {}, {}, "${menu}", _${group}Group\n};`);
      }
    }
  }

  /**
   * generate the code to instantiate the shared constraints
   */
  constraints() {
    if ("constraints" in this.device.desc) {
      let constraints = this.device.desc.constraints;
      for (let oid in constraints) {
        this.device.constraints[oid] = new Constraint(oid, constraints[oid]);
        bloc(this.device.constraints[oid] .getInitializer());
      }
    }
  }

  /**
   * generate the code to instantiate the params
   */
  params() {
    if (!"params" in this.device.desc) {
      return;
    }
    for (let oid in this.device.desc.params) {
      // handle special case for product param
      if (oid == "product") {
        // we need to add code to overwrite the value of catena_sdk_version with
        // whatever's up-to-date in the SDK.
        // this is done by adding some code to the postscript
        cloc(`#define STRINGIFY(x) #x`);
        cloc(`#define TO_STRING(x) STRINGIFY(x)`);
        cloc(`constexpr const char* real_sdk_version = TO_STRING(CATENA_CPP_VERSION);`);
        cloc(`${this.device.namespace}::Product& initialize_sdk_version(${this.device.namespace}::Product& p) {`);
        cloc(`p.catena_sdk_version = real_sdk_version;`,1);
        cloc(`return p;`,1);
        cloc(`}`);
        cloc(`${this.device.namespace}::Product dummy = initialize_sdk_version(product);`);
      }

      // add the param to the device
      let param = this.device.params[oid] = new Param(oid, this.device.desc.params[oid], this.device.namespace, this.device);

      // define the param in the header file
      if (param.hasTypeInfo()) {
        this.writeTypeInfo(param);
      }

      // initialize the param in the body file
      if (param.hasValue()) {
        // write param initial value
        bloc(param.initializeValue());
        // write param descriptors
        this.writeConstraintsAndDescriptors(param);
        // inititalize the ParamWithValue object
        bloc(param.initializeParamWithValue());
      }
    }
  }

  /**
   * Recursively write the struct/variant type info for a param and its subparams in the header file
   * @param {Param} param the param to write type info for
   */
  writeTypeInfo(param) {
    if (param.isStructType() && !param.isTemplated()) {
      // write struct definition
      let defType = param.isArrayType() ? param.elementType() : param.objectType();
      hloc(`struct ${defType} {`, hindent++);
      for (let subParam of param.getSubParams()) {
        if (subParam.hasTypeInfo()) {
          this.writeTypeInfo(subParam);
        }
        hloc(subParam.initializeValue(), hindent);
      }
      hloc(`using isCatenaStruct = void;`, hindent);
      hloc(`};`, --hindent);

      // add StructInfo specialization to the buffer
      ploc(`template<>`);
      ploc(`struct catena::common::StructInfo<${param.objectNamespaceType()}> {`, pindent++);
      ploc(`using ${param.objectType()} = ${param.objectNamespaceType()};`, pindent);
      ploc(`using Type = std::tuple<${param.getFieldInfoTypes()}>;`, pindent);
      ploc(`static constexpr Type fields = {${param.getFieldInfoInit()}};`, pindent);
      ploc(`};`, --pindent);
      

    } else if (param.isVariantType() && !param.isTemplated()) {
      // define subparams for the variant
      hloc(`namespace _${param.oid} {`, hindent++);
      let subParamCount = 0;
      for (let subParam of param.getSubParams()) {
        subParamCount++;
        if (subParam.hasTypeInfo()) {
          this.writeTypeInfo(subParam);
        }
      }
      hloc(`} // namespace _${param.oid}`, --hindent);

      // write variant type alias
      let defType = param.isArrayType() ? param.elementType() : param.objectType();
      hloc(`using ${defType} = std::variant<${param.getAlternativeTypes()}>;`, hindent);

      // write AlternativeNames specialization to the buffer
      ploc(`template<>`);
      ploc(`inline std::array<const char*, ${subParamCount}> catena::common::alternativeNames<${param.namespace}::${defType}>{${param.getAlternativeNames()}};`);
    }

    // write vector type Alias
    if (param.isArrayType() && !param.template_param?.isArrayType()) {
      hloc(`using ${param.objectType()} = std::vector<${param.elementType()}>;`, hindent);
    }
  }

   /**
   * Recursively writes the constraints and descriptors for a param and its subparams
   * @param {Param} param the param to write constraints and descriptors for
   * @param {string} parentVarName the variable name of the parent param
   */
   writeConstraintsAndDescriptors(param, parentVarName = "") {
    if (param.constraint != undefined && !param.constraint.isInitialized()) {
      bloc(param.constraint.getInitializer());
    }
    let varName = `${parentVarName}_${param.oid}`;
    bloc(`catena::common::ParamDescriptor ${varName}Descriptor(${param.descriptor.getArgs(parentVarName)});`);
    if (param.hasTypeInfo()) {
      for (let subParam of param.getSubParams()) {
        this.writeConstraintsAndDescriptors(subParam, varName);
      }
    }
  }

  /**
   * generate the code to instantiate the commands
   */
  commands() {
    if (!"commands" in this.device.desc) {
      return;
    }
    for (let oid in this.device.desc.commands) {
      // add the command to the device
      let command = this.device.commands[oid] = new Param(oid, this.device.desc.commands[oid], this.device.namespace, this.device, undefined, true);

      // define the command in the header file
      if (command.hasTypeInfo()) {
        this.writeTypeInfo(command);
      }

      // initialize the command in the body file
      if (command.hasValue()) {
        bloc(command.initializeValue());
      }
      this.writeConstraintsAndDescriptors(command);
      bloc(command.initializeParamWithValue());
    }
  }

  /**
   * close the device namespace and write the buffer
   */
  finish = () => {
    hloc(`} // namespace ${this.device.namespace}`);
    postscript();
    coda();
  };
}

module.exports = CppGen;
