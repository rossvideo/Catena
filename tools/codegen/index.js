/*Copyright 2025 Ross Video Ltd
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
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import { program } from 'commander';
import fs from 'fs';
import path from "path";

import packageJson from './package.json' with { type: "json" };;
import { DeviceModel } from './DeviceModel.js';
import Validator from 'smpte-validator';

//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
program
    .description(packageJson.description)
    .version(packageJson.version)
    .option('-s, --schema <string>', 'path to schema definitions', '../../smpte/interface/schemata/device.json')
    .option('-d, --device-model <string>', 'Catena device model to process', '../../example_device_models/device.minimal.json')
    .option('-l, --language <string>', 'Language to generate code for', 'cpp')
    .option('-o, --output <string>', 'Output folder for generated code', '.')
    .option('--disable-mandatory-enforcement', 'Disable enforcement of mandatory parameters during code generation')
    .option('-q, --quiet', 'Suppress non-error console output', false)

program.parse();
const options = program.opts();
// define our logging functions to be used throughout
// if quiet mode is enabled, log does nothing
const log = options.quiet ? function() {} : console.log;
const error = console.error;

if (options.schema) {
    log(`schema: ${options.schema}`);
}
if (options.deviceModel) {
    log(`deviceModel: ${options.deviceModel}`);
}
if (options.language) {
    log(`language: ${options.language}`);
}
if (options.output) {
    log(`output: ${options.output}`);
}
if (options.disableMandatoryEnforcement) {
    log(`Mandatory parameter enforcement disabled`);
}

// verify input file exists
if (!fs.existsSync(options.deviceModel)) {
    error(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}

// extract schema name from input filename
const deviceName = path.parse(options.deviceModel).name.split('.')[0];
log(`Validating device model '${deviceName}' from file '${options.deviceModel}' against schema file '${options.schema}'...`);

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
        throw new Error(`Missing mandatory product struct in params`);
    }

    if (deviceParams.product.type !== 'STRUCT') {
        throw new Error(`Product parameter must be STRUCT type, not ${deviceParams.product.type}`);
    }

    if (!deviceParams.product.read_only) {
        throw new Error(`Product parameter must be read_only`);
    }

    const productParams = deviceParams.product.params || {};
    const productValue = deviceParams.product.value;
    const missing = [];
    const emptyValues = [];

    Object.entries(REQUIRED_PARAMS).forEach(([key, checkValue]) => {
        const param = productParams[key];

        if (!param) {
            missing.push(key);
        } else {
            if (param.type !== 'STRING') {
                missing.push(`${key} (not STRING type)`);
                return;
            }

            if (checkValue) {
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
        }
    });

    const allIssues = [...missing.map(p => `${p} (missing field)`), ...emptyValues];

    if (allIssues.length > 0 && !disableMandatoryEnforcement) {
        throw new Error(`Invalid mandatory product parameters: ${allIssues.join(', ')}`);
    }
}

(async () => {
    const validator = new Validator(options.schema);
    log(`Applying schema '${deviceName}' to file '${options.deviceModel}'`);
    const isValid = validator.validate(deviceName, options.deviceModel);
    if (isValid.data) {
        const dm = new DeviceModel(options.deviceModel, validator, isValid.data);

        areAllRequiredParamsPresent(dm.desc.params, options.disableMandatoryEnforcement);

        log('✅ Validation succeeded.');
        log("Generating code...");
        const { default: CodeGen } = await import(`../../tools/codegen/${options.language}/${options.language}gen.js`);
        const codeGen = new CodeGen(dm, options.output);

        codeGen.generate();
        log('✅ Code generation completed.');
    } else {
        error('❌ Schema validation failed.');
        process.exit(1);
    }
})().catch((err) => {
    error(`Error: ${err.message}`);
    process.exit(err.error || 1);
});
