const CppCtor = require('./cppctor');

/**
 * 
 * @param {string} s 
 * @returns input with the first letter capitalized
 */
function initialCap(s) {s
    return s.charAt(0).toUpperCase() + s.slice(1);
}

function typeArg(desc) {
    const types = {
        STRING: `std::string`,
        INT32: `int32_t`,
        FLOAT32: `float`,
        STRING_ARRAY: `std::vector<std::string>`,
        INT32_ARRAY: `std::vector<int32_t>`,
        FLOAT32_ARRAY: `std::vector<float>`,
        STRUCT: `${initialCap(this.oid)}`,
        STRUCT_ARRAY: `std::vector<${initialCap(this.oid)}>`
    }
    if (desc.type in types) {
        this.type = types[desc.type];
    } else {
        throw new Error(`Unknown type ${desc.type}`);
    }
    return `catena::ParamType::${desc.type}`;
}

function repeatString() {
    return this;
}

function oidAliasesArg(desc) {
    let ans = `{}`;
    if ("oid_aliases" in desc) {
        ans = `{${desc.oid_aliases.map(alias => `{"${alias}"}`).join(', ')}}`;
    }
    return ans;
}

function nameArg(desc) {
    let ans = `{}`;
    if ("name" in desc) {
        ans = '{';
        const display_strings = desc.name.display_strings;
        const n = Object.keys(display_strings).length;
        let i = 0;
        for (let lang in display_strings) {
            ans += `{"${lang}", "${display_strings[lang]}"}`;
            if (i++ < n-1) { ans += ','; }
        }   
        ans += '}';
    }
    return ans;
}

function widgetArg (desc) {
    let ans = `{}`;
    if ("widget" in desc) {
        ans = `"${desc.widget}"`;
    }
    return ans
}

function readOnly (desc) {
    let ans = `false`;
    if ("read_only" in desc) {
        ans = desc.read_only ? `true` : `false`;
    }
    return ans;
}

function jpointerArg(desc) {
    return `"/${this.oid}"`;
}

function parentArg(desc) {
    return `dm`;
}

/**
 * Create constructor arguments for catena::lite::Device object
 * @param {object} desc device descriptor
 */
class Param extends CppCtor {
    constructor(oid, desc) {
        super(desc[oid]);
        this.oid = oid;
        this.deviceParams = desc;
        this.init = '{}';
        this.arguments.push(typeArg.bind(this));
        this.arguments.push(repeatString.bind(this.oid));
        this.arguments.push(oidAliasesArg);
        this.arguments.push(nameArg);
        this.arguments.push(widgetArg);
        this.arguments.push(readOnly);
        this.arguments.push(jpointerArg.bind(this));
        this.arguments.push(parentArg);
    }

    initializer (desc) {
        const valueObject = {
            string_value: (value) => { return `"${value}"`; },
            int32_value: (value) => { return `${value}`; },
            float32_value: (value) => { return `${value}`; },
            string_array_values: (value) => { return `${value.strings.map(v => `"${v}"`).join(', ')}`; },
            int32_array_values: (value) => { return `${value.ints.join(', ')}`; },
            float32_array_values: (value) => { return `${value.floats.join(', ')}`; },
            struct_value: (value) => {
                let fields = value.fields;
                let fieldsArr = Object.keys(fields);
                // recursively call the correct valueObject function on each field
                let mappedFields = fieldsArr.map(
                    field => {
                        let key = Object.keys(fields[field].value)[0];
                        return valueObject[key](fields[field].value[key]);
                    }
                );
                return mappedFields.join(',');
            },
            struct_array_values: (value) => {
                let arr = value.struct_values;
                let mappedArr = arr.map(
                    item => {
                        let fields = item.fields;
                        let fieldsArr = Object.keys(fields);
                        let mappedFields = fieldsArr.map(
                            field => {
                                let key = Object.keys(fields[field].value)[0];
                                return valueObject[key](fields[field].value[key]);
                            }
                        );
                        return `{${mappedFields.join(',')}}`;
                    }
                );
                return mappedArr.join(',');
            }

        }
        if ("value" in desc) {
            let key = Object.keys(desc.value)[0];
            if (key in valueObject) {
                this.init = `{${valueObject[key](desc.value[key])}}`;
            } else {
                throw new Error(`Unknown value type ${key}`);
            }   
            return this.init;
        }
    }

    objectName () {
        return this.oid;
    }

    objectType () {
        return this.type;
    }

    params (hloc, bloc) {
        if ("params" in this.desc) {

        }
    }
}

module.exports = Param;