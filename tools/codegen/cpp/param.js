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
const Constraint = require("./constraint");

/**
 *
 * @param {string} s
 * @returns input with the first letter capitalized
 */
function initialCap(s) {
  return s.charAt(0).toUpperCase() + s.slice(1);
}

/**
 *
 * @param {object} desc param descriptor
 * @returns c++ type of the param's value as a string
 */
function typeArg(type) {
  const types = {
    STRING: `std::string`,
    INT32: `int32_t`,
    FLOAT32: `float`,
    STRING_ARRAY: `std::vector<std::string>`,
    INT32_ARRAY: `std::vector<int32_t>`,
    FLOAT32_ARRAY: `std::vector<float>`,
    EMPTY: `EmptyValue`,
  };

  if (type in types) {
    return types[type];
  } else {
    throw new Error(`Unknown type ${type}`);
  }
}

function valueTypeArg(type) {
  const types = {
    STRING: `string_value`,
    INT32: `int32_value`,
    FLOAT32: `float32_value`,
    STRING_ARRAY: `string_array_values`,
    INT32_ARRAY: `int32_array_values`,
    FLOAT32_ARRAY: `float32_array_values`,
    STRUCT: `struct_value`,
    STRUCT_ARRAY: `struct_array_values`,
    STRUCT_VARIANT: `struct_variant_value`,
    STRUCT_VARIANT_ARRAY: `struct_variant_array_values`,
  };

  if (type in types) {
    return types[type];
  } else {
    throw new Error(`Unknown type ${type}`);
  }
}

function removeArraySuffix(type) {
  return type.replace("_ARRAY", "");
}

// /**
//  * @param {string} s
//  * @returns whatever string is bound to this
//  */
// function quoted(s) {
//   return `"${s}"`;
// }

// /**
//  *
//  * @param {object} desc param descriptor
//  * @returns initializer for the oid_aliases member
//  */
// function oidAliasesArg(desc) {
//   let ans = `{}`;
//   if ("oid_aliases" in desc) {
//     ans = `{${desc.oid_aliases.map((alias) => `{"${alias}"}`).join(", ")}}`;
//   }
//   return ans;
// }

// /**
//  *
//  * @param {object} desc param descriptor
//  * @returns display strings initializer
//  */
// function nameArg(desc) {
//   let ans = `{}`;
//   if ("name" in desc) {
//     ans = "{";
//     const display_strings = desc.name.display_strings;
//     const n = Object.keys(display_strings).length;
//     let i = 0;
//     for (let lang in display_strings) {
//       ans += `{"${lang}", "${display_strings[lang]}"}`;
//       if (i++ < n - 1) {
//         ans += ",";
//       }
//     }
//     ans += "}";
//   }
//   return ans;
// }

// /**
//  *
//  * @param {object} desc param descriptor
//  * @returns the widget in quotes if present, otherwise an empty initializer
//  */
// function widgetArg(desc) {
//   let ans = `{}`;
//   if ("widget" in desc) {
//     ans = `"${desc.widget}"`;
//   }
//   return ans;
// }

// /**
//  * 
//  * @param {object} desc param descriptor
//  * @returns param's access scope
//  */
// function accessScope(desc) {
//   let ans;
//   if ("access_scope" in desc) {
//     ans = `"${desc.access_scope}"`;
//   } else {
//     ans = `""`;
//   }
//   return ans;
// }

// /**
//  *
//  * @param {object} desc param descriptor
//  * @returns true or false reflecting presence and state of the read_only flag.
//  */
// function readOnly(desc) {
//   let ans = `false`;
//   if ("read_only" in desc) {
//     ans = desc.read_only ? `true` : `false`;
//   }
//   return ans;
// }

// function jpointerArg(desc) {
//   return `"${this.parentOid}/${this.oid}"`;
// }

class Descriptor {
  constructor(desc, oid, constraintOid, parentOid = "", isCommand) {
    const args = {
      type: () => { 
        return `catena::ParamType::${desc.type}`;
      },
      oid_aliases: () => {
        return `{${(desc.oid_aliases ?? []).map((alias) => `{"${alias}"}`).join(", ")}}`;
      },
      name: () => {
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
      },
      widget: () => {
        return `"${desc.widget || ""}"`;
      },
      access_scope: () => {
        return `"${desc.access_scope || ""}"`;
      },
      read_only: () => {
        return !!desc.readonly;
      },
      oid: () => {
        return `"${oid}"`;
      },
      constraint: () => {
        return constraintOid ? `&${constraintOid}` : "nullptr";
      },
      is_command: () => {
        return !!isCommand;
      },
      device: () => {
        return "dm";
      }
    };

    this.oid = oid;
    this.parentOid = parentOid;
    const argsArray = Object.values(args);
    this.args = argsArray.map(arg => arg()).join(', ');
  }

  getArgs(parentVarName) {
    let parentArg = parentVarName ? `&${parentVarName}Descriptor` : "nullptr";
    return `${this.args}, ${parentArg}`;
  }
}

class Param {
  /**
   * @param {desc} desc The param descriptor
   */
  constructor(oid, desc, namespace, device, parent = undefined, isCommand = false) {
    if ("import" in desc) {
      desc = device.deviceModel.importParam(desc.import);
    }

    this.oid = oid;
    this.namespace = namespace;
    this.subParams = {};
    this.type = desc.type;
    this.value = desc.value;
    this.varName = parent ? `${parent.varName}_${oid}` : `_${oid}`;
    this.isCommand = isCommand;


    if ("constraint" in desc) {
      if (desc.constraint.ref_oid) {
        if (!device.hasConstraint(desc.constraint.ref_oid)) {
          throw new Error(`Shared constraint ${desc.constraint.ref_oid} not found`);
        }
        this.constraintVar = `constraints_${desc.constraint.ref_oid}Constraint`;
      } else {
        this.constraint = new Constraint("", this.varName, desc.constraint);
        this.constraintVar = `_${oid}Constraint`;
      }
    }

    if ("template_oid" in desc) {
      this.template_param = device.getParam(desc.template_oid); 
      if (this.template_param == undefined) {
        throw new Error(`Template param ${desc.template_oid} not found`);
      }

      if (this.template_param.type != this.type && this.template_param.type != removeArraySuffix(this.type)) {
        throw new Error(`Param with type ${this.type} can not be templated on param with type ${this.template_param.type}`);
      }
      
      // If param doesn't have a constraint, use the template's constraint
      if (this.constraintVar == undefined) {
        this.constraintVar = this.template_param.constraintVar;
      }
    }

    this.descriptor = new Descriptor(desc, oid, this.constraint?.oid, parent?.oid, isCommand);

    if ("params" in desc) {
      if (!this.hasTypeInfo()) {
        throw new Error(`${this.type} type can not have subparams`);
      }
      for (let oid in desc.params) {
        this.subParams[oid] = new Param(oid, desc.params[oid], `${namespace}::${initialCap(this.oid)}`, device, this);
      }
    }
  }

  getParam(path) {
    let front = path.shift();
    if (!front in this.subParams) {
      return undefined;
    }

    if (path.length == 0) {
      return this.subParams[front];
    } else {
      return this.subParams[front].getParam(path);
    }
  }

  isTemplated() {
    return this.template_param != undefined;
  }

  hasTypeInfo() {
    return this.isStructType() || this.isVariantType();
  }

  isStructType() {
    return this.type == "STRUCT" || this.type == "STRUCT_ARRAY";
  }

  isVariantType() {
    return this.type == "STRUCT_VARIANT" || this.type == "STRUCT_VARIANT_ARRAY";
  }

  isArrayType() {
    return this.type.includes("ARRAY");
  }

  hasValue() {
    return this.value != undefined;
  }

  hasUniqueConstraint() {
    return this.constraint != undefined;
  }

  objectType() {
    if (!this.hasTypeInfo()) {
      return typeArg(this.type);
    }

    if (this.isArrayType() && (!this.isTemplated() || this.template_param.isArrayType())) {
      return this.template_param.objectType();
    } else {
      return `${initialCap(this.oid)}`;
    }
  }

  objectNamespaceType() {
    if (!this.hasTypeInfo()) {
      return typeArg(this.type);
    }
    if (this.isArrayType() && (!this.isTemplated() || this.template_param.isArrayType())) {
      return this.template_param.objectNamespaceType();
    }
    return `${this.namespace}::${initialCap(this.oid)}`;
  }

  elementType() {
    if (!this.isArrayType()) {
      throw new Error(`${this.type} type does not have an element type`);
    }

    if (!this.isTemplated()) {
      return `${this.objectType()}_elem`;
    } else if (!this.template_param.isArrayType()) {
      return `${this.template_param.objectNamespaceType()}`;
    } else {
      return this.template_param.elementType();
    }
  }

  initializeValue() {
    if (!this.hasValue()) {
      return `${this.objectType()} ${this.oid};`;
    }
    let param = this.template_param || this;
    return `${this.objectType()} ${this.oid}${this.valueInitializer(this.value, this.type, param)};`;
  }

  valueInitializer(value, type, param) {
    const valueObject = {
      // simple types are pretty simple to handle
      string_value: (typeValue) => {
        return `"${typeValue}"`;
      },
      int32_value: (typeValue) => {
        return `${typeValue}`;
      },
      float32_value: (typeValue) => {
        return `${typeValue}`;
      },
      string_array_values: (typeValue) => {
        return `${typeValue.strings.map((v) => `"${v}"`).join(", ")}`;
      },
      int32_array_values: (typeValue) => {
        return `${typeValue.ints.join(", ")}`;
      },
      float32_array_values: (typeValue) => {
        return `${typeValue.floats.join(", ")}`;
      },
      // structs and struct arrays are more complex
      struct_value: (typeValue) => {
        if (typeValue.fields == undefined) {
          throw new Error("Struct value must have fields map");
        }
        let fieldsArr = Object.keys(typeValue.fields);
        let mappedFields = fieldsArr.map((field) => {
          let subParam = param.getParam([field]);
          if (subParam == undefined) {
            throw new Error(`Subparam ${field} not found`);
          }
          let paramDef = subParam.template_param || subParam;
          return `.${field}${this.valueInitializer(typeValue.fields[field].value, subParam.type, paramDef)}`;
        });
        return `{${mappedFields.join(",")}}`;
      },
      struct_array_values: (typeValue) => {
        let arr = typeValue.struct_values;
        let mappedArr = arr.map((item) => {
          if (item.struct_value == undefined) {
            throw new Error("struct_array_value must have struct_value field");
          }
          return `${valueObject.struct_value(item.struct_value)}`;
        });
        return `${mappedArr.join(",")}`;
      },
      struct_variant_value: (typeValue) => {
        if (typeValue.struct_varaint_type == undefined) {
          throw new Error("struct_variant_value must have struct_variant_type");
        }
        let subParam = param.getParam(typeValue.struct_variant_type);
        if (subParam == undefined) {
          throw new Error(`${typeValue.struct_variant_type} is not an alternative of ${param.oid}`);
        }
        if (typeValue.value == undefined) {
          throw new Error("struct_variant_value must have value field");
        }
        return `${valueInitializer(typeValue.value, subParam)}`;
      },
      struct_variant_array_values: (typeValue) => {
        let arr = typeValue.struct_variants;
        let mappedArr = arr.map(valueObject.struct_variant_value);
        return `${mappedArr.join(",")}`;
      },
    };

    let key = Object.keys(value)[0];
    if (key != valueTypeArg(type)) {
      throw new Error(`Value type ${key} does not match param type ${valueTypeArg(type)}`);
    }
    return `{${valueObject[key](value[key])}}`;
  }

  initializeParamWithValue() {
    return `catena::common::ParamWithValue<${this.objectNamespaceType()}> ` +
           `_${this.oid}Param(${this.oid}, _${this.oid}Descriptor, dm, ${this.isCommand});`;
  }

  getSubParams() {
    if (!this.hasTypeInfo()) {
      throw new Error(`${this.type} type does not have subparams`);
    }

    if (this.isTemplated() && this.template_param.hasTypeInfo()) {
      return this.template_param.getSubParams();
    } else {
      return Object.values(this.subParams);
    }
  }

  getFieldInfoTypes() {
    let subParamArr = Object.values(this.subParams);
    let mappedArr = subParamArr.map((subParam) => {
      return `FieldInfo<${subParam.objectNamespaceType()}, ${this.objectType()}>`;
    });
    return mappedArr.join(", ");
  }

  getFieldInfoInit() {
    let subParamArr = Object.values(this.subParams);
    let mappedArr = subParamArr.map((subParam) => {
      return `{"${subParam.oid}", &${this.objectType()}::${subParam.oid}}`;
    });
    return mappedArr.join(", ");
  }

// class Param extends CppCtor {
//   /**
//    * Create constructor arguments for catena::common::Param object
//    * @param {Array} parentOid array of ancestors' oids
//    * @param {string} oid object id of the param being processed
//    * @param {object} desc descriptor of parent object
//    */
//   // constructor(parentOid, oid, desc) {
//   //   super(desc[oid]);
//   //   this.parentOid = parentOid;
//   //   this.oid = oid;
//   //   this.deviceParams = desc;
//   //   this.init = "{}";
//   //   this.template_oid = "template_oid" in desc[oid] ? desc[oid].template_oid : "";
//   //   this.constraint = "constraint" in desc[oid] ? desc[oid].constraint : "";
//   //   this.arguments.push(typeArg.bind(this));
//   //   this.arguments.push(oidAliasesArg);
//   //   this.arguments.push(nameArg);
//   //   this.arguments.push(widgetArg);
//   //   this.arguments.push(accessScope);
//   //   this.arguments.push(readOnly);
//   //   this.arguments.push(quoted(this.oid));
//   // }

//   /**
//    *
//    * @returns the oid of the param
//    */
//   objectName() {
//     return this.oid;
//   }

//   /**
//    *
//    * @returns c++ type of the param's value
//    */
//   objectType() {
//     return this.type;
//   }

//   /**
//    *
//    * @returns true if the param has subparams, false otherwise
//    */
//   hasSubparams() {
//     return "params" in this.desc;
//   }

//   /**
//    * 
//    * @returns true if the param has a value, false otherwise
//    */
//   hasValue() {
//     return "value" in this.desc;
//   }

//   isArrayType() {
//     return this.desc.type.includes("ARRAY");
//   }

//   /**
//    *
//    * @returns true if the param is a struct or struct array type, false otherwise
//    */
//   isStructType() {
//     return (
//       this.desc.type == "STRUCT" ||
//       this.desc.type == "STRUCT_ARRAY"
//     );
//   }

//   /**
//    *
//    * @returns true if the param is a struct variant or struct variant array type, false otherwise
//    */
//   isVariantType() {
//     return (
//       this.desc.type == "STRUCT_VARIANT" ||
//       this.desc.type == "STRUCT_VARIANT_ARRAY"
//     );
//   }

//   /**
//    *
//    * @returns unique C++ legal identifier for the Param
//    */
//   paramName() {
//     return `${this.parentName()}_${this.oid}`;
//   }

//   parentName() {
//     return this.parentOid.replace(/\//g, "_");
//   }

//   isTemplated() {
//     return this.template_oid != "";
//   }

//   templateOid() {
//     return this.template_oid;
//   } 

//   /**
//    *
//    * @returns true if the param has a constraint, false otherwise
//    */
//   isConstrained() {
//     return this.constraint != "";
//   }

//   /**
//    *
//    * @returns true if the param's constraint is shared, false otherwise
//    */
//   usesSharedConstraint() {
//     return this.constraint != "" && "ref_oid" in this.constraint;
//   }

//   /**
//    *
//    * @returns returns the oid of the shared constraint
//    */
//   constraintRef() {
//     return `_${this.constraint.ref_oid}`;
//   }
}

module.exports = Param;
