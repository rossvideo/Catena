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


const CppCtor = require("./cppctor");

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
 *
 * @param {object} desc param descriptor
 * @returns c++ type of the param's value as a string
 */
function typeArg(desc) {
  const types = {
    STRING: `std::string`,
    INT32: `int32_t`,
    FLOAT32: `float`,
    STRING_ARRAY: `std::vector<std::string>`,
    INT32_ARRAY: `std::vector<int32_t>`,
    FLOAT32_ARRAY: `std::vector<float>`,
    STRUCT: `${initialCap(this.oid)}`,
    STRUCT_ARRAY: `${initialCap(this.oid)}`,
    EMPTY: `EmptyValue`,
  };

  if (desc.type in types) {
    this.type = types[desc.type];
  } else {
    throw new Error(`Unknown type ${desc.type}`);
  }
  return `catena::ParamType::${desc.type}`;
}

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
 * @returns initializer for the oid_aliases member
 */
function oidAliasesArg(desc) {
  let ans = `{}`;
  if ("oid_aliases" in desc) {
    ans = `{${desc.oid_aliases.map((alias) => `{"${alias}"}`).join(", ")}}`;
  }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns display strings initializer
 */
function nameArg(desc) {
  let ans = `{}`;
  if ("name" in desc) {
    ans = "{";
    const display_strings = desc.name.display_strings;
    const n = Object.keys(display_strings).length;
    let i = 0;
    for (let lang in display_strings) {
      ans += `{"${lang}", "${display_strings[lang]}"}`;
      if (i++ < n - 1) {
        ans += ",";
      }
    }
    ans += "}";
  }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns the widget in quotes if present, otherwise an empty initializer
 */
function widgetArg(desc) {
  let ans = `{}`;
  if ("widget" in desc) {
    ans = `"${desc.widget}"`;
  }
  return ans;
}

/**
 * 
 * @param {object} desc param descriptor
 * @returns param's access scope
 */
function accessScope(desc) {
  let ans;
  if ("access_scope" in desc) {
    ans = `"${desc.access_scope}"`;
  } else {
    ans = `""`;
  }
  return ans;
}

/**
 *
 * @param {object} desc param descriptor
 * @returns true or false reflecting presence and state of the read_only flag.
 */
function readOnly(desc) {
  let ans = `false`;
  if ("read_only" in desc) {
    ans = desc.read_only ? `true` : `false`;
  }
  return ans;
}

function jpointerArg(desc) {
  return `"${this.parentOid}/${this.oid}"`;
}

class Param extends CppCtor {
  /**
   * Create constructor arguments for catena::lite::Param object
   * @param {Array} parentOid array of ancestors' oids
   * @param {string} oid object id of the param being processed
   * @param {object} desc descriptor of parent object
   */
  constructor(parentOid, oid, desc) {
    super(desc[oid]);
    this.parentOid = parentOid;
    this.oid = oid;
    this.deviceParams = desc;
    this.init = "{}";
    this.template_oid = "template_oid" in desc[oid] ? desc[oid].template_oid : "";
    this.constraint = "constraint" in desc[oid] ? desc[oid].constraint : "";
    this.arguments.push(typeArg.bind(this));
    this.arguments.push(oidAliasesArg);
    this.arguments.push(nameArg);
    this.arguments.push(widgetArg);
    this.arguments.push(accessScope);
    this.arguments.push(readOnly);
    this.arguments.push(quoted.bind(this.oid));
  }

  /**
   *
   * @param {object} desc param descriptor of param being processed (can be recursive)
   * @returns initializer for the param's value object
   */
  initializer(desc) {
    const valueObject = {
      // simple types are pretty simple to handle
      string_value: (value) => {
        return `"${value}"`;
      },
      int32_value: (value) => {
        return `${value}`;
      },
      float32_value: (value) => {
        return `${value}`;
      },
      string_array_values: (value) => {
        return `${value.strings.map((v) => `"${v}"`).join(", ")}`;
      },
      int32_array_values: (value) => {
        return `${value.ints.join(", ")}`;
      },
      float32_array_values: (value) => {
        return `${value.floats.join(", ")}`;
      },

      // structs and struct arrays are more complex
      struct_value: (value, isStructChild = false) => {
        let fields = value.fields;
        let fieldsArr = Object.keys(fields);
        // recursively call the correct valueObject function on each field
        let mappedFields = fieldsArr.map((field) => {
          let key = Object.keys(fields[field].value)[0];
          return valueObject[key](fields[field].value[key], true);
        });

        return isStructChild
          ? `{${mappedFields.join(",")}}`
          : mappedFields.join(",");
      },
      struct_array_values: (value, isStructChild = false) => {
        let arr = value.struct_values;
        let mappedArr = arr.map((item) => {
          let fields = item.struct_value.fields;
          let fieldsArr = Object.keys(fields);
          let mappedFields = fieldsArr.map((field) => {
            let key = Object.keys(fields[field].value)[0];
            return valueObject[key](fields[field].value[key], true);
          });
          return `{${mappedFields.join(",")}}`;
        });
        return isStructChild 
          ? `{${mappedArr.join(",")}}`
          : mappedArr.join(",");
      },
    };
    if ("value" in desc) {
      let key = Object.keys(desc.value)[0];
      if (key in valueObject) {
        this.init = `{${valueObject[key](desc.value[key])}}`;
      } else {
        throw new Error(`Unknown value type ${key}`);
      }
    }
    return this.init;
  }

  /**
   *
   * @returns the oid of the param
   */
  objectName() {
    return this.oid;
  }

  /**
   *
   * @returns c++ type of the param's value
   */
  objectType() {
    return this.type;
  }

  /**
   *
   * @returns true if the param has subparams, false otherwise
   */
  hasSubparams() {
    return "params" in this.desc;
  }

  /**
   * 
   * @returns true if the param has a value, false otherwise
   */
  hasValue() {
    return "value" in this.desc;
  }

  isArrayType() {
    return this.desc.type.includes("ARRAY");
  }

  /**
   *
   * @returns true if the param is a struct or struct array type, false otherwise
   */
  isStructType() {
    return (
      this.desc.type == "STRUCT" ||
      this.desc.type == "STRUCT_ARRAY"
    );
  }

  /**
   *
   * @returns true if the param is a struct variant or struct variant array type, false otherwise
   */
  isVariantType() {
    return (
      this.desc.type == "STRUCT_VARIANT" ||
      this.desc.type == "STRUCT_VARIANT_ARRAY"
    );
  }

  /**
   *
   * @returns unique C++ legal identifier for the Param
   */
  paramName() {
    return `${this.parentName()}_${this.oid}`;
  }

  parentName() {
    return this.parentOid.replace(/\//g, "_");
  }

  isTemplated() {
    return this.template_oid != "";
  }

  templateOid() {
    return this.template_oid;
  } 

  /**
   *
   * @returns true if the param has a constraint, false otherwise
   */
  isConstrained() {
    return this.constraint != "";
  }

  /**
   *
   * @returns true if the param's constraint is shared, false otherwise
   */
  usesSharedConstraint() {
    return this.constraint != "" && "ref_oid" in this.constraint;
  }

  /**
   *
   * @returns returns the oid of the shared constraint
   */
  constraintRef() {
    return `_${this.constraint.ref_oid}`;
  }
}

module.exports = Param;
