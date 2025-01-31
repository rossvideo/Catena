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
 * converts a catena type to a c++ type for simple types
 * @param {string} type the catena type of the param
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

/**
 * converts a catena type to the corresponding value type argument
 * @param {string} type the catena type of the param
 * @returns the value type of the param as a string
 */
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

/**
 * @param {string} type the catena type of the param
 * @returns the non-array version of the type
 */
function removeArraySuffix(type) {
  return type.replace("_ARRAY", "");
}

/**
 * @class Descriptor
 * @brief extracts the paramDescriptor arguments from the param description
 * @param {object} desc the param description
 * @param {string} oid the object id of the param
 * @param {Constraint} constraint the constraint object
 * @param {string} parentOid the object id of the parent param
 * @param {boolean} isCommand true if the param is a command, false otherwise
 */
class Descriptor {
  constructor(desc, oid, constraint, parentOid = "", isCommand) {
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
        return constraint ? `&${constraint.variableName()}` : "nullptr";
      },
      is_command: () => {
        return !!isCommand;
      },
      device: () => {
        return "dm";
      },
      max_length: () => {
        return ("max_length" in desc) ? `${desc.max_length}` : "0";
      }
    };

    this.oid = oid;
    this.parentOid = parentOid;
    const argsArray = Object.values(args);
    this.args = argsArray.map(arg => arg()).join(', ');
  }

  /**
   * @param {string} parentVarName the variable name of the parent param descriptor
   * @returns the arguments for the ParamDescriptor constructor as a string
   */
  getArgs(parentVarName) {
    let parentArg = parentVarName ? `&${parentVarName}Descriptor` : "nullptr";
    return `${this.args}, ${parentArg}`;
  }
}

/**
 * @class Param
 * @brief maintains the inforamtion for a single parameter
 * @param {string} oid the object id of the param
 * @param {object} desc the param description
 * @param {string} namespace the namespace of the param
 * @param {Device} device the device object
 * @param {Param} parent the parent param object
 * @param {boolean} isCommand true if the param is a command, false otherwise
 * @throws if the param references an invalid template param, constraint, or imported param
 */
class Param {
  constructor(oid, desc, namespace, device, parent = undefined, isCommand = false) {
    if ("import" in desc) {
      // overwrite desc with the imported param description
      desc = device.deviceModel.importParam(desc.import);
    }

    this.oid = oid;
    this.namespace = namespace;
    this.subParams = {};
    this.type = isCommand ? "EMPTY" : desc.type;
    this.value = desc.value;
    this.isCommand = isCommand;
    this.parent = parent;
    //this.maxLength = ("max_length" in desc) ? desc.max_length : 0;

    if ("constraint" in desc) {
      if (desc.constraint.ref_oid) {
        this.constraint = device.getConstraint(desc.constraint.ref_oid);
      } else {
        this.constraint = new Constraint(oid, desc.constraint, this);
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
      if (this.constraint == undefined) {
        this.constraint = this.template_param.constraint;
      }
    }

    this.descriptor = new Descriptor(desc, oid, this.constraint, parent?.oid, isCommand);

    if ("params" in desc) {
      if (!this.hasTypeInfo()) {
        throw new Error(`${this.type} type can not have subparams`);
      }
      for (let oid in desc.params) {
        let subParamNamespace = this.isVariantType() ? `${this.namespace}::_${this.oid}` : `${this.namespace}::${initialCap(this.oid)}`;
        this.subParams[oid] = new Param(oid, desc.params[oid], `${subParamNamespace}`, device, this);
      }
    }
  }

  /**
   * gets the parameter object at the end of the path
   * @param {string[]} path an array of the oids in the path
   * @returns the param object pointed to by the path
   */
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

  /**
   * 
   * @returns true if the param is templated, false otherwise
   */
  isTemplated() {
    return this.template_param != undefined;
  }

  /**
   * 
   * @returns true if the param is a struct or variant type, false otherwise
   */
  hasTypeInfo() {
    return this.isStructType() || this.isVariantType();
  }

  /**
   * 
   * @returns true if the param is a struct or struct array type, false otherwise
   */
  isStructType() {
    return this.type == "STRUCT" || this.type == "STRUCT_ARRAY";
  }

  /**
   * 
   * @returns true if the param is a struct variant or struct variant array type, false otherwise
   */
  isVariantType() {
    return this.type == "STRUCT_VARIANT" || this.type == "STRUCT_VARIANT_ARRAY";
  }

  /**
   * 
   * @returns true if the param is an array type, false otherwise
   */
  isArrayType() {
    return this.type.includes("ARRAY");
  }

  /**
   * 
   * @returns true if the param has defined value, false otherwise
   */
  hasValue() {
    return this.value != undefined;
  }

  /**
   * 
   * @returns the c++ type of the param's value as a string
   */
  objectType() {
    if (!this.hasTypeInfo()) {
      return typeArg(this.type);
    }
    if (this.isTemplated()) {
      if (this.isArrayType() && !this.template_param.isArrayType()) {
        return `${initialCap(this.oid)}`;
      } else {
        return this.template_param.objectType();
      }
    } else {
      return `${initialCap(this.oid)}`;
    }
  }

  /**
   * 
   * @returns the c++ type of the param's value with it's full namespace as a string
   */
  objectNamespaceType() {
    if (!this.hasTypeInfo()) {
      return typeArg(this.type);
    }
    if (this.isTemplated()) {
      if (this.isArrayType() && !this.template_param.isArrayType()) {
        return `${this.namespace}::${initialCap(this.oid)}`;
      } else {
        return this.template_param.objectNamespaceType();
      }
    } else {
      return `${this.namespace}::${initialCap(this.oid)}`;
    }
  }

  /**
   * 
   * @returns the c++ type of an element of an array type param
   * @throws if the param is not an array type
   */
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

  /**
   * 
   * @returns a string to initialize the param's value
   * Example: 'std::string hello{"Hello, World!"};'
   */
  initializeValue() {
    if (!this.hasValue()) {
      return `${this.objectType()} ${this.oid};`;
    }
    let param = this.template_param || this;
    return `${this.objectType()} ${this.oid}${this.valueInitializer(this.value, this.type, param)};`;
  }

  /**
   * @param {object} value the value object from the param description
   * @param {string} type the catena type of the value object
   * @param {Param} param the param object that the value is associated with
   * @returns the params value initializer as a string
   * Example: '{"Hello, World!"}'
   * @throws if the value type does not match the param type
   */
  valueInitializer(value, type, param) {
    const valueObject = {
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

      struct_value: (typeValue) => {
        if (typeValue.fields == undefined) {
          throw new Error("Struct value must have fields map");
        }
        let fieldsArr = Object.keys(param.subParams); // get keys from the param to maintain definition order
        let mappedFields = [];
        for (let field of fieldsArr) {
          if (field in typeValue.fields) { // The value may not initialize all fields
            let subParam = param.getParam([field]);
            if (subParam == undefined) {
              throw new Error(`Subparam ${field} not found`);
            }
            let paramDef = subParam.template_param || subParam;
            mappedFields.push(`.${field}${this.valueInitializer(typeValue.fields[field].value, subParam.type, paramDef)}`);
          }
        }
        return `${mappedFields.join(",")}`;
      },

      struct_array_values: (typeValue) => {
        let arr = typeValue.struct_values;
        let mappedArr = arr.map((item) => {
          if (item.struct_value == undefined) {
            throw new Error("struct_array_value must have struct_value field");
          }
          return `{${valueObject.struct_value(item.struct_value)}}`;
        });
        return `${mappedArr.join(",")}`;
      },

      struct_variant_value: (typeValue) => {
        if (typeValue.struct_variant_type == undefined) {
          throw new Error("struct_variant_value must have struct_variant_type");
        }
        let subParam = param.getParam([typeValue.struct_variant_type]);
        if (subParam == undefined) {
          throw new Error(`${typeValue.struct_variant_type} is not an alternative of ${param.oid}`);
        }
        if (typeValue.value == undefined) {
          throw new Error("struct_variant_value must have value field");
        }
        let paramDef = subParam.template_param || subParam;
        return `${paramDef.objectNamespaceType()}${this.valueInitializer(typeValue.value, subParam.type, paramDef)}`;
      },

      struct_variant_array_values: (typeValue) => {
        let arr = typeValue.struct_variants;
        let mappedArr = arr.map(valueObject.struct_variant_value);
        return `${mappedArr.join(",")}`;
      },

      undefined: (typeValue) => {
        return "";
      }
    };

    let key = Object.keys(value)[0];
    if (key != undefined && key != valueTypeArg(type)) {
      throw new Error(`Value type ${key} does not match param type ${valueTypeArg(type)}`);
    }
    return `{${valueObject[key](value[key])}}`;
  }

  /**
   * 
   * @returns the string to initialize the ParamWithValue object
   */
  initializeParamWithValue() {
    let valueVar = this.isCommand ? "catena::common::emptyValue" : this.oid;
    return `catena::common::ParamWithValue<${this.objectNamespaceType()}> ` +
           `_${this.oid}Param(${valueVar}, _${this.oid}Descriptor, dm, ${this.isCommand});`;
  }

  /**
   * 
   * @returns A list of Param objects that are subparams of this param
   */
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

  /**
   * 
   * @returns A string that declares the type of each field in the StructInfo tuple
   */
  getFieldInfoTypes() {
    let subParamArr = Object.values(this.subParams);
    let mappedArr = subParamArr.map((subParam) => {
      return `FieldInfo<${subParam.objectNamespaceType()}, ${this.objectType()}>`;
    });
    return mappedArr.join(", ");
  }

  /**
   * 
   * @returns A string that initializes the fields in the StructInfo tuple
   */
  getFieldInfoInit() {
    let subParamArr = Object.values(this.subParams);
    let mappedArr = subParamArr.map((subParam) => {
      return `{"${subParam.oid}", &${this.objectType()}::${subParam.oid}}`;
    });
    return mappedArr.join(", ");
  }

  /**
   * 
   * @returns A string that declares the type of each alternative of the variant type
   */
  getAlternativeTypes() {
    if (!this.isVariantType()) {
      throw new Error(`${this.type} type does not have alternatives`);
    }

    let alternatives = Object.values(this.subParams);
    let mappedArr = alternatives.map((alternative) => {
      return `${alternative.objectNamespaceType()}`;
    });
    return mappedArr.join(", ");
  }

  /**
   * 
   * @returns A string that list the oids of each alternative of the variant type
   */
  getAlternativeNames() {
    if (!this.isVariantType()) {
      throw new Error(`${this.type} type does not have alternatives`);
    }

    let alternativeOids = Object.keys(this.subParams);
    let mappedArr = alternativeOids.map((alternativeOid) => {
      return `"${alternativeOid}"`;
    });
    return mappedArr.join(", ");
  }

  /**
   * 
   * @returns The fully qualified oid of this param
   */
  getFQOid() {
    if (this.parent != undefined) {
      return `${this.parent.getFQOid()}/${this.oid}`;
    } else {
      return `/${this.oid}`;
    }
  }
}

module.exports = Param;
