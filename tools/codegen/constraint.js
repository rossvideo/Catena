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
  let range_desc;
  if (this.constrainedType === "int32_t") {
    range_desc = desc.int_range;
  } else {
    range_desc = desc.float_range;
  }
  return range_desc.min_value;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns max value of the range constraint
 */
function maxArg(desc) {
  let range_desc;
  if (this.constrainedType === "int32_t") {
    range_desc = desc.int_range;
  } else {
    range_desc = desc.float_range;
  }
  return range_desc.max_value;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns step value of the range constraint.
 * defaults to 1
 */
function stepArg(desc) {
  let ans = 1;
  if ("step" in desc && this.constrainedType === "int32_t") {
    ans = desc.int_range.step;
  } else if ("step" in desc) {
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
  let range_desc;
  if (this.constrainedType === "int32_t") {
    range_desc = desc.int_range;
  } else {
    range_desc = desc.float_range;
  }

  let ans = range_desc.min_value;
  if ("display_min" in range_desc) {
    ans = range_desc.display_min;
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
  let range_desc;
  if (this.constrainedType === "int32_t") {
    range_desc = desc.int_range;
  } else {
    range_desc = desc.float_range;
  }

  let ans = range_desc.max_value;
  if ("display_max" in range_desc) {
    ans = range_desc.display_max;
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
  let ans = `false`;
  if ("strict" in desc) {
    ans = desc.strict ? `true` : `false`;
  }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns true or false if the constraint is shared
 */
function sharedArg(desc) {
  return this.shared;
}

class Constraint extends CppCtor {
  /**
   * Create constructor arguments for catena::lite::Constraint object
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
    this.arguments.push(parentArg.bind(this));
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
