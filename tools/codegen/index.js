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
const { program } = require('commander');  //Loads the commander Library to handle cmd line arguments 
program //Defines cmd line options (Default values are provided for each)
    .option('-s, --schema <string>', 'path to schema definitions', '../../smpte/interface/schemata/device.json') //Schema file path
    .option('-d, --device-model <string>', 'Catena device model to process', '../../example_device_models/device.minimal.json') //JSON device model input file path
    .option('-l, --language <string>', 'Language to generate code for', 'cpp') // code generation language
    .option('-o, --output <string>', 'Output folder for generated code', '../../build/cpp') // where to output the generated code
    .option('-m, --mandatory-params <string>', 'patht to files')
    .option('disable-mandatory-enforcement', 'Disable enforcement of mandatory parameters during code generation' , true)
    // Individual mandatory parameter flags
    .option('--product-name <string>', 'Catena')
    .option('--product-vendor <string>', 'Ross Video')
    .option('--product-version <string>', '1.0.0')
    .option('--catena-sdk <string> ' , '')
    .option('--catena-sdk-version <string> ' , '')
    .option('--product-serial <string>', 'SN-7K9M-2024-XR485-BLU')

program.parse(process.argv); //Parses the cmd line arguments
const options = program.opts(); //Stores them in options
if (options.schema) { //Logs out each option (for debugging or confirmation purposes)
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
if(options.mandatoryParams) {
    console.log(`mandatoryParams: ${options.mandatoryParams}`);
}
if(options.disableMandatoryEnforcement) {
    console.log(`Mandatory parameter enforcement disabled`);
}
// import the fs libraries
const fs = require('fs');

// verify input file exists (makes sure the specified device model JSON file exists and exits if it doesn't)
if (!fs.existsSync(options.deviceModel)) {
    console.log(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}

//These libraries are used for validation, path handling, and optional YAML import
const Validator = require('smpte-validator');
const validator = new Validator(options.schema);
const path = require("node:path");
const yaml = require('yaml')

/**
 * @class DeviceModel
 * @brief Holds the information parsed from a json device model file
 */

//Encapsulates a device model, its validator, and methods to import additional files
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

      //Allows importing other JSON/YAML files referenced in the main device model
      importParam(importArg) {
          if ("file" in importArg) {
            const importDir = path.dirname(this.filePath);
            const importPath = `${importDir}/${importArg.file}`;
            if (!fs.existsSync(importPath)) {
              throw new Error(`Cannot open file at ${importArg.file}`);
            }
            // Determining if the file is yaml or json and parsing accordingly.
            const importData = (() => {
                const extension = path.extname(importPath);
                if (extension === ".yaml" || extension === ".yml") {
                    return yaml.parse(fs.readFileSync(importPath, 'utf8'));
                } else { // Default
                    return JSON.parse(fs.readFileSync(importPath));
                }
            })();
            return importData;
      
          } else {
            throw new Error(`Unsupported import type: ${importArg}`);
          }
      }
}

// extract schema name from input filename
const deviceName = path.parse(options.deviceModel).name.split('.')[0]; //Extracts the name prefix (eg device.minimal.json -> device)
console.log(`Validating device model '${deviceName}' from file '${options.deviceModel}' against schema file '${options.schema}'...`); //Begins validation

/**
 * Apply CLI argument overrides to mandatory parameters
 * @param {Object} deviceData the device model data
 * @param {Object} options parsed CLI options
 */
function applyCliOverrides(deviceData, options) {
    // Product parameter mappings: CLI option -> device model field
    const productOverrides = {
        productName: 'name',
        productVendor: 'vendor', 
        productVersion: 'version',
        productSerial: 'serial_number',
        catenaSdk: 'catena_sdk',
        catenaSdkVersion: 'catena_sdk_version'
    };
    
    if (!deviceData.params) deviceData.params = {};
    if (!deviceData.params.product) {
        deviceData.params.product = { type: "STRUCT", read_only: true, params: {} };
    }
    if (!deviceData.params.product.params) deviceData.params.product.params = {};
    
    // Ensure SDK parameters always have values (for C++ struct initialization)  
    const sdkDefaults = {
        catena_sdk: "",
        catena_sdk_version: ""
    };
    
    Object.entries(sdkDefaults).forEach(([paramName, defaultValue]) => {
        if (!deviceData.params.product.params[paramName]) {
            deviceData.params.product.params[paramName] = { type: "STRING" };
        }
        if (!deviceData.params.product.params[paramName].value) {
            deviceData.params.product.params[paramName].value = { string_value: defaultValue };
            
            // Also set in YAML structure if it exists
            if (deviceData.params.product.value?.struct_value?.fields) {
                deviceData.params.product.value.struct_value.fields[paramName] = {
                    string_value: defaultValue
                };
            }
        }
    });
    
    const hasOverrides = Object.keys(productOverrides).some(key => options[key]);
      
    if (hasOverrides) {
        // Apply all product parameter overrides with detailed output
        Object.entries(productOverrides).forEach(([cliKey, fieldKey]) => {
            if (options[cliKey]) {
                // Get original value for comparison
                let originalValue = "undefined";
                
                // Check JSON structure first
                if (deviceData.params.product.params?.[fieldKey]?.value?.string_value) {
                    originalValue = deviceData.params.product.params[fieldKey].value.string_value;
                }
                // Check YAML structure
                else if (deviceData.params.product.value?.struct_value?.fields?.[fieldKey]?.string_value) {
                    originalValue = deviceData.params.product.value.struct_value.fields[fieldKey].string_value;
                }
                
                // Show what's being overridden
                console.log(`🔄 Overriding product.${fieldKey}: "${originalValue}" → "${options[cliKey]}"`);
                
                // Apply JSON structure
                deviceData.params.product.params[fieldKey] = { 
                    type: "STRING", value: { string_value: options[cliKey] }
                };
                
                // Apply YAML structure (if exists)
                if (deviceData.params.product.value?.struct_value) {
                    if (!deviceData.params.product.value.struct_value.fields) {
                        deviceData.params.product.value.struct_value.fields = {};
                    }
                    deviceData.params.product.value.struct_value.fields[fieldKey] = { 
                        string_value: options[cliKey] 
                    };
                }
            }
        });
    }
    
    // Apply device-level overrides
    if (options.deviceSlot !== undefined) {
        const originalSlot = deviceData.slot || "undefined";
        console.log(`🔄 Overriding device.slot: "${originalSlot}" → "${options.deviceSlot}"`);
        deviceData.slot = options.deviceSlot;
    }
}

function areAllRequiredParamsPresent(deviceParams, disableMandatoryEnforcement = false) {
  const REQUIRED_PARAMS = [
    "name",
    "vendor",
    "version",
    "catena_sdk",
    "catena_sdk_version",
    "serial_number"
  ];

  //console.log('🔍 DEBUG areAllRequiredParamsPresent called - hasProduct:', !!(deviceParams && deviceParams.product));

  // Check if product struct exists
  if (!deviceParams || !deviceParams.product) {
    if (!disableMandatoryEnforcement) {
      throw new Error(`Missing mandatory product struct in params`);
    }
    return;
  }

  // Check if product has params and values
  const productParams = deviceParams.product.params || {};
  const productValue = deviceParams.product.value; 
  const missing = [];
  const emptyValues = [];

  REQUIRED_PARAMS.forEach(key => {
    const param = productParams[key];
    
    if (!param) {
      missing.push(key);
    } else if (key !== 'catena_sdk_version' && key !== 'catena_sdk') { 
    // Check for values in both JSON and YAML structures
    let stringValue;

    // JSPM strucutre: product.params[key].value.string_value
    if (param.value && param.value.string_value) {
        stringValue = param.value.string_value;
    }
    // YAML structure: product.value.struct_value.fields[key].string_value
    else if (productValue && productValue.struct_value && productValue.struct_value.fields) {
        const field = productValue.struct_value.fields[key];
        if (field && field.string_value) {
            stringValue = field.string_value;
        }
    }

        // Validate the found value
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

  console.log('✅ All mandatory product parameters present');
}

try {
    const validator = new Validator(options.schema);
    console.log(`Applying schema '${deviceName}' to file '${options.deviceModel}'`);
    const isValid = validator.validate(deviceName, options.deviceModel);
    
    let validationPassed = isValid.valid;
    let mandatoryParamsValid = true;
    
    if (isValid.data) {
        // Apply CLI overrides BEFORE creating DeviceModel and validation
        applyCliOverrides(isValid.data, options);

        const dm = new DeviceModel(options.deviceModel, validator, isValid.data);
        
        // Check mandatory parameters 
        try {
            areAllRequiredParamsPresent(dm.desc.params, options.disableMandatoryEnforcement);
        } catch (err) {
            mandatoryParamsValid = false;
            console.log(`❌ ${err.message}`);
            validationPassed = false;
        }
        
        // Show overall validation result and exit early if failed
        const shouldContinue = validationPassed && (mandatoryParamsValid || options.disableMandatoryEnforcement);
        
        if (shouldContinue) {
            if (validationPassed && mandatoryParamsValid) {
                console.log('✅ Validation succeeded.');
            } else if (options.disableMandatoryEnforcement && !mandatoryParamsValid) {
                console.log('⚠️  Validation issues found but enforcement disabled - continuing with code generation.');
            }
        } else {
            console.log('❌ Validation failed.');
            if (!validationPassed) {
                console.log('❌ Code generation stopped due to schema validation errors.');
            } else {
                console.log('❌ Code generation stopped due to mandatory parameter errors.');
            }
            process.exit(1);
        }
        
        // Continue with code generation even if mandatory params are missing (build compatibility)
        const CodeGen =  require(`../../tools/codegen/${options.language}/${options.language}gen.js`);
        const codeGen = new CodeGen(dm, options.output);
        
        if (!mandatoryParamsValid) {
            console.log('ℹ️  Code generation will continue - this may affect runtime functionality');
        }
            
        console.log("Generating code...");
        codeGen.generate();
        console.log('✅ Code generation completed.');
    } else {
        console.log('❌ Validation failed.');
    }
} catch (err) {
    console.error(`Error: ${err.message}`);
    process.exit(err.error || 1);
} // Catches and logs any validation or file read errors