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
  return this.shared ? "dm" : `&${this.oid}Param`;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns min value of the range constraint
 */
function minArg(desc) {
  return desc.float_range.min_value;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns max value of the range constraint
 */
function maxArg(desc) {
  return desc.float_range.max_value;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns step value of the range constraint.
 * defaults to 1
 */
function stepArg(desc) {
  let ans = 1;
  if ("step" in desc) {
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
  let ans = desc.float_range.min_value;
  if ("display_min" in desc) {
    ans = desc.float_range.display_min;
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
  let ans = desc.float_range.max_value;
  if ("display_max" in desc) {
    ans = desc.float_range.display_max;
  }
  return ans;
}

function namedChoicesArg(desc) {
  let ans = "{}";
  return ans;
}

function choicesArg(desc) {
  let ans = "{}";
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
   * @param {boolean} shared indicates whether the constraint is shared
   * @param {string} oid object id of the param being processed
   * @param {object} desc descriptor of parent object
   */
  constructor(shared, oid, desc) {
    super(desc[oid]);
    this.shared = shared;
    this.oid = oid;
    this.findType(desc[oid]);
    if (this.constraintType === "RangeConstraint") {
      this.arguments.push(minArg);
      this.arguments.push(maxArg);
      this.arguments.push(stepArg);
      this.arguments.push(displayMinArg);
      this.arguments.push(displayMaxArg);
    } else if (this.constraintType === "NamedChoiceConstraint") {
      this.arguments.push(namedChoicesArg);
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
   * @param {object} desc constraint descriptor
   * @returns c++ type of the constraint's value as a string
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

    const fields = {
      RangeConstraint: ["min", "max", "step", "display_min", "display_max"],
      NamedChoiceConstraint: ["named_choices", "strict"],
      PicklistConstraint: ["choices", "strict"],
      ALARM_TABLE: [], // not yet implemented
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
   * @returns the c++ type of the constraint
   */
  objectType() {
    return this.constraintType;
  }

  /**
   *
   * @returns the c++ type being constrained by this constraint
   */
  constrainedType() {
    return this.type;
  }

  /**
   *
   * @returns true if the param has subparams, false otherwise
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
    return `_${this.oid}`;
  }
}

module.exports = Constraint;
