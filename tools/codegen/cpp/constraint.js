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

/**
 * file: constraint.js
 * brief: Collect C++ information for a constraint
 * author: isaac.robert@rossvideo.com
 * date: 2024-08-21
 * Copyright (c) 2024 Ross Video
 */

"use strict";

const CppCtor = require("./cppctor");

/**
 *
 * @returns whatever string is bound to this
 */
function quoted() {
  return `"${this}"`;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns the owner of the constraint
 */
function parentArg(desc) {
  return this.shared ? "dm" : `&${this.parentOid}_${this.oid}Param`;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns min value of the range constraint
 */
function minArg(desc) {
  if (this.type === "int32_t") {
    return desc.int32_range.min_value;
  } else {
    return desc.float_range.min_value;
  }
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns max value of the range constraint
 */
function maxArg(desc) {
  if (this.type === "int32_t") {
    return desc.int32_range.max_value;
  } else {
    return desc.float_range.max_value;
  }
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns step value of the range constraint.
 * 
 * If not present, step is set to 0
 */
function stepArg(desc) {
  let ans = 0;
  if (this.type == "int32_t" && "step" in desc.int32_range) {
    ans = desc.int32_range.step;
  } else if (this.type == "float" && "step" in desc.float_range) {
    ans = desc.float_range.step;
  }
  return ans;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns display min value of the range constraint.
 * defaults to min value
 */
function displayMinArg(desc) {
  let ans;
  if (this.type === "int32_t") {
    if ("display_min" in desc.int32_range) {
      ans = desc.int32_range.display_min;
    } else {
      ans = desc.int32_range.min_value;
    }
  } else {
    if ("display_min" in desc.float_range) {
      ans = desc.float_range.display_min;
    } else {
      ans = desc.float_range.min_value;
    }
  }
  return ans;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns display max value of the range constraint.
 * defaults to max value
 */
function displayMaxArg(desc) {
  let ans;
  if (this.type === "int32_t") {
    if ("display_max" in desc.int32_range) {
      ans = desc.int32_range.display_max;
    } else {
      ans = desc.int32_range.max_value
    }
  } else {
    if ("display_max" in desc.float_range) {
      ans = desc.float_range.display_max;
    } else {
      ans = desc.float_range.max_value;
    }
  }
  return ans;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns the choices for the named choice constraint
 */
function namedChoicesArg(desc) {
  let choices;
  if (this.type === "int32_t") {
    choices = desc.int32_choice.choices;
  } else {
    choices = desc.string_string_choice.choices;
  }
  let ans = '{';
    for (let i = 0; i < choices.length; ++i) {
      if (this.type === "int32_t") {
        ans += `{${choices[i].value},{`;
      } else {
        ans += `{"${choices[i].value}",{`;
      }
      let display_strings = choices[i].name.display_strings;
      let pairs = Object.keys(display_strings).length;
      for (let lang in display_strings) {
        ans += `{"${lang}","${display_strings[lang]}"}`;
        if (--pairs > 0) {
          ans += ',';
        }
      }
      ans += '}}';
      if (i < choices.length - 1) {
        ans += ',';
      } else {
        ans += '}';
      }
    }
  return ans;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns the choices for the picklist constraint
 */
function choicesArg(desc) {
  let choices = desc.string_choice.choices;
  let ans = '{';
    for (let i = 0; i < choices.length; ++i) {
      ans += `"${choices[i]}"`;
      if (i < choices.length - 1) {
        ans += ',';
      } else {
        ans += '}';
      }
    }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns true or false reflecting presence and state of the strict flag.
 */
function strictArg(desc) {
  // @todo int_choice might need to default to true
  let ans = `false`;
  if (desc.type === "STRING_CHOICE") {
    if ("strict" in desc.string_choice) {
      ans = desc.string_choice.strict ? `true` : `false`;
    }
  } else if (desc.type === "STRING_STRING_CHOICE") {
    if ("strict" in desc.string_string_choice) {
      ans = desc.string_string_choice.strict ? `true` : `false`;
    }
  }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns true or false if the constraint is shared
 * If shared, devive model reference is passed as an argument
 */
function sharedArg(desc) {
  if (this.shared) {
    return "true, dm";
  } else {
    return "false";
  }
}

class Constraint extends CppCtor {
  /**
   * Create constructor arguments for catena::common::Constraint object
   * @param {string} parentOid oid of the parent object, empty if owned by device
   * @param {string} oid object id of the constraint being processed
   * @param {object} desc descriptor of parent object
   */
  constructor(parentOid, oid, desc) {
    // structure of shared constraints vary from param constraints
    let this_desc = desc[oid];
    if (parentOid !== "constraints") {
      this_desc = this_desc.constraint;
    }
    super(this_desc);
    this.shared = parentOid === "constraints";
    this.parentOid = parentOid;
    this.oid = oid;
    // arguments change based on constraint type
    this.findType(this_desc);
    if (this.constraintType === "RangeConstraint") {
      this.arguments.push(minArg.bind(this));
      this.arguments.push(maxArg.bind(this));
      this.arguments.push(stepArg.bind(this));
      this.arguments.push(displayMinArg.bind(this));
      this.arguments.push(displayMaxArg.bind(this));
    } else if (this.constraintType === "NamedChoiceConstraint") {
      this.arguments.push(namedChoicesArg.bind(this));
      this.arguments.push(strictArg);
    } else if (this.constraintType === "PicklistConstraint") {
      this.arguments.push(choicesArg);
      this.arguments.push(strictArg);
    }
    this.arguments.push(quoted.bind(this.oid));
    this.arguments.push(sharedArg.bind(this));
  }

  /**
   *
   * Identifies the constraint type
   * @param {object} desc constraint descriptor
   */
  findType(desc) {
    const types = {
      INT_RANGE: ["int32_t", "RangeConstraint"],
      FLOAT_RANGE: ["float", "RangeConstraint"],
      INT_CHOICE: ["int32_t", "NamedChoiceConstraint"],
      STRING_STRING_CHOICE: ["std::string", "NamedChoiceConstraint"],
      STRING_CHOICE: ["std::string", "PicklistConstraint"],
      ALARM_TABLE: ["", ""], // not yet implemented
    };

    if (desc.type in types) {
      this.type = types[desc.type][0];
      this.constraintType = types[desc.type][1];
    } else if (desc.type === "ALARM_TABLE") {
      throw new Error(`Alarm table not yet implemented`);
    } else {
      throw new Error(`Unknown type ${desc.type}`);
    }
  }

  /**
   *
   * @returns the C++ type of the constraint
   */
  objectType() {
    if (this.constraintType === "PicklistConstraint") {
      return "PicklistConstraint";
    } else {
      return `${this.constraintType}<${this.type}>`;
    }
  }

  /**
   *
   * @returns true if the constraint is owned by the device, 
   * false if owned by a param 
   */
  isShared() {
    return this.shared;
  }

  /**
   *
   * @returns the oid of the constraint
   */
  objectName() {
    return this.oid;
  }

  /**
   *
   * @returns unique C++ legal identifier for the constraint
   */
  constraintName() {
    return `${this.parentName()}_${this.oid}`;
  }

  parentName() {
    return this.parentOid.replace(/\//g, "_");
  }
}

module.exports = Constraint;
