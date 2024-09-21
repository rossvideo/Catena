/*
Copyright © 2024 Ross Video Limited, All Rights Reserved.
 
Licensed under the Creative Commons Attribution NoDerivatives 4.0
International Licensing (CC-BY-ND-4.0);
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:
 
https://creativecommons.org/licenses/by-nd/4.0/
 
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
const { program } = require('commander');
program
    .option('-s, --schema <string>', 'path to schema definitions', '../../schema/catena.schema.json')
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




// import the path and fs libraries
const path = require('node:path');
const fs = require('fs');

// verify input file exists
if (!fs.existsSync(options.deviceModel)) {
    console.log(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}


// read the schema definition file
const schemaFilename = options.schema;

const Validator = require('./validator.js');

const validator = new Validator(schemaFilename);

try {
 
    // read the file to validate
    const data = JSON.parse(fs.readFileSync(options.deviceModel));
    if (validator.validate('device', data)) {
        const CodeGen =  require(`./${options.language}/${options.language}gen.js`);
        const codeGen = new CodeGen(options.deviceModel, options.output, validator, data);
        codeGen.generate();
    }

} catch (why) {
    console.log(why.message);
    process.exit(typeof why.error === 'number' ? why.error : 1);
}
