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

//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
const { program } = require('commander');
program
    .option('-s, --schema <string>', 'path to schema definitions', '../../smpte/interface/schemata/device.json')
    .option('-d, --device-model <string>', 'Catena device model to process', '../../example_device_models/device.minimal.json')
    .option('-l, --language <string>', 'Language to generate code for', 'cpp')
    .option('-o, --output <string>', 'Output folder for generated code', '.')
    .option('--disable-mandatory-enforcement', 'Disable enforcement of mandatory parameters during code generation')

program.parse(process.argv);
const options = program.opts();
if (options.schema) {
    console.log(`schema: ${options.schema}`);
}
if (options.deviceModel) {
    console.log(`deviceModel: ${options.deviceModel}`);
}
if (options.language) {
    console.log(`language: ${options.language}`);
}
if (options.output) {
    console.log(`output: ${options.output}`);
}
if (options.disableMandatoryEnforcement) {
    console.log(`Mandatory parameter enforcement disabled`);
}

const fs = require('fs');

if (!fs.existsSync(options.deviceModel)) {
    console.log(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}

const Validator = require('smpte-validator');
const validator = new Validator(options.schema);
const path = require("node:path");
const yaml = require('yaml')

/**
 * @class DeviceModel
 * @brief Holds the information parsed from a json device model file
 */
class DeviceModel {
    /**
     * @brief Construct a new DeviceModel object
     * @param {string} filePath the path to the device model file
     * @param {Validator} validator the json validator object
     * @param {object} desc the parsed json object
     */
    constructor(filePath, validator, desc) {
        this.filePath = filePath;
        this.validator = validator;
        this.desc = desc;
        this.baseFilename = path.basename(filePath);
        const info = this.baseFilename.split(".");
        const schemaName = info[0];
        if (schemaName !== "device") {
            throw new Error(`File must be a device model, not ${schemaName}`);
        }
        this.deviceName = info[1];
    }

    /**
     * @brief open a param.*.json file and return the parsed json object
     * @param {string} importArg the path to the param.*.json file relative to the directory containing the device model file
     * @returns the parsed json object
     * @throws {Error} if the file cannot be opened or the data is invalid against the schema
     * @todo add support for other import types 
     */
    importParam(importArg) {
        if ("file" in importArg) {
            const importDir = path.dirname(this.filePath);
            const importPath = `${importDir}/${importArg.file}`;
            if (!fs.existsSync(importPath)) {
                throw new Error(`Cannot open file at ${importArg.file}`);
            }

            const importData = (() => {
                const extension = path.extname(importPath);
                if (extension === ".yaml" || extension === ".yml") {
                    return yaml.parse(fs.readFileSync(importPath, 'utf8'));
                } else { 
                    return JSON.parse(fs.readFileSync(importPath));
                }
            })();
            return importData;

        } else {
            throw new Error(`Unsupported import type: ${importArg}`);
        }
    }
}

const deviceName = path.parse(options.deviceModel).name.split('.')[0];

/**
 * @brief Validates that all mandatory product parameters are present and have valid values
 * @param {object} deviceParams The device parameters object from the device model
 * @param {boolean} disableMandatoryEnforcement If true, skip validation and return early
 * @throws {Error} If mandatory parameters are missing or have invalid values (when enforcement enabled)
 */
function areAllRequiredParamsPresent(deviceParams, disableMandatoryEnforcement = false) {
    if (disableMandatoryEnforcement) {
        return;
    }
    const REQUIRED_PARAMS = {
        "name": true,
        "vendor": true,
        "version": true,
        "catena_sdk": false,
        "catena_sdk_version": false,
        "serial_number": true
    };

    if (!deviceParams || !deviceParams.product) {
        return;
    }

    const productParams = deviceParams.product.params || {};
    const productValue = deviceParams.product.value;
    const missing = [];
    const emptyValues = [];

    Object.entries(REQUIRED_PARAMS).forEach(([key, checkValue]) => {
        const param = productParams[key];

        if (!param) {
            missing.push(key);
        } else if (checkValue) {
            if (param.type !== 'STRING') {
                missing.push(`${key} (not STRING type)`);
                return;
            }

            let stringValue;

            if (productValue && productValue.struct_value && productValue.struct_value.fields) {
                const field = productValue.struct_value.fields[key];
                if (field && field.string_value) {
                    stringValue = field.string_value;
                }
            }

            if (!stringValue) {
                emptyValues.push(`${key} (no value found)`);
            } else if (stringValue.trim() === '') {
                emptyValues.push(`${key} (empty string value)`);
            }
        }
    });

    const allIssues = [...missing.map(p => `${p} (missing field)`), ...emptyValues];

    if (allIssues.length > 0 && !disableMandatoryEnforcement) {
        throw new Error(`Invalid mandatory product parameters: ${allIssues.join(', ')}`);
    }
}

try {
    const validator = new Validator(options.schema);
    const isValid = validator.validate(deviceName, options.deviceModel);

    if (isValid.data) {
        const dm = new DeviceModel(options.deviceModel, validator, isValid.data);

        areAllRequiredParamsPresent(dm.desc.params, options.disableMandatoryEnforcement);

        console.log('✅ Validation succeeded.');
        console.log("Generating code...");
        const CodeGen = require(`../../tools/codegen/${options.language}/${options.language}gen.js`);
        const codeGen = new CodeGen(dm, options.output);

        codeGen.generate();
        console.log('✅ Code generation completed.');
    } else {
        console.log('❌ Schema validation failed.');
        process.exit(1);
    }
} catch (err) {
    console.error(`Error: ${err.message}`);
    process.exit(err.error || 1);
}