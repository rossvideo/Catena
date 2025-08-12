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
    .option('-o, --output <string>', 'Output folder for generated code', '.');

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

// import the fs libraries
const fs = require('fs');

// verify input file exists
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
const deviceName = path.parse(options.deviceModel).name.split('.')[0];
console.log(`Validating device model '${deviceName}' from file '${options.deviceModel}' against schema file '${options.schema}'...`);

// try {
//     // Determining if the file is yaml or json and parsing accordingly.
//     const data = (() => {
//         const extension = path.extname(options.deviceModel);
//         if (extension === ".yaml" || extension === ".yml") {
//             return yaml.parse(fs.readFileSync(options.deviceModel, 'utf8'));
//         } else { // Default
//             return JSON.parse(fs.readFileSync(options.deviceModel));
//         }
//     })();
//     if (validator.validateDevice(data)) {
//         const CodeGen =  require(`./${options.language}/${options.language}gen.js`);
//         const dm = new DeviceModel(options.deviceModel, validator, data);
//         const codeGen = new CodeGen(dm, options.output);
//         codeGen.generate();
//     }

// } catch (why) {
//     console.log(why.message);
//     process.exit(typeof why.error === 'number' ? why.error : 1);
// }


try {
    const validator = new Validator(options.schema);
    console.log(`Applying schema '${deviceName}' to file '${options.deviceModel}'`);
    const isValid = validator.validate(deviceName, options.deviceModel);
    console.log(isValid.valid ? '✅ Validation succeeded.' : '❌ Validation failed.');
    if (isValid.data) {
        const CodeGen =  require(`../../tools/codegen/${options.language}/${options.language}gen.js`);
        const dm = new DeviceModel(options.deviceModel, validator, isValid.data);
        const codeGen = new CodeGen(dm, options.output);
        console.log("Generating code...");
        codeGen.generate();
        console.log('✅ Code generation completed.');
    }
} catch (err) {
    console.error(`Error: ${err.message}`);
    process.exit(err.error || 1);
}