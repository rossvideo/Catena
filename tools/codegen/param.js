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
    STRUCT_ARRAY: `std::vector<${initialCap(this.oid)}>`,
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

function parentArg(desc) {
  return this.parentOid == "" ? "dm" : `&${this.parentName()}Param`;
}

/**
 * Create constructor arguments for catena::lite::Device object
 * @param {Array} array of ancestors' oids
 * @param {string} oid object id of the param being processed
 * @param {object} desc descriptor of parent object
 */
class Param extends CppCtor {
  constructor(parentOid, oid, desc) {
    super(desc[oid]);
    this.parentOid = parentOid;
    this.oid = oid;
    this.deviceParams = desc;
    this.init = "{}";
    this.arguments.push(typeArg.bind(this));
    this.arguments.push(oidAliasesArg);
    this.arguments.push(nameArg);
    this.arguments.push(widgetArg);
    this.arguments.push(readOnly);
    this.arguments.push(quoted.bind(this.oid));
    this.arguments.push(parentArg.bind(this));
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
          let fields = item.fields;
          let fieldsArr = Object.keys(fields);
          let mappedFields = fieldsArr.map((field) => {
            let key = Object.keys(fields[field].value)[0];
            return valueObject[key](fields[field].value[key], true);
          });
          return isStructChild
            ? `{${mappedFields.join(",")}}`
            : mappedFields.join(",");
        });
        return mappedArr.join(",");
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
   * @returns true if the param is a struct or struct array type, false otherwise
   */
  isStructType() {
    return (
      this.desc.type == "catena::ParamType::STRUCT" ||
      this.desc.type == "catena::ParamType::STRUCT_ARRAY"
    );
  }

  /**
   *
   * @returns true if the param is a struct variant or struct variant array type, false otherwise
   */
  isVariantType() {
    return (
      this.desc.type == "catena::ParamType::STRUCT_VARIANT" ||
      this.desc.type == "catena::ParamType::STRUCT_VARIANT_ARRAY"
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

  fieldInfo() {
    return {
      name: this.oid,
      typename: this.type,
    };
  }
}

module.exports = Param;
