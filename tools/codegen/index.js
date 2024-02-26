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

// load the validation engine
const Ajv = require('ajv');
const addFormats = require('ajv-formats').default;
const ajv = new Ajv({strict: false});  
addFormats(ajv);

// our use of "strict" as a schema interferes with ajv's strict mode.
// so we only turn it on after loading ajv
'use strict';

// load the command line parser
const { program } = require('commander');
program
    .option('-d, --device-model <string>', 'Catena device model to process', "../../example_device_models/device.minimal.json")
    .option('-l, --language <string>', 'Language to generate code for', "cpp")
    .option('-o, --output <string>', 'Output folder for generated code', '.');

program.parse(process.argv);
const options = program.opts();
if (options.deviceModel) {
    console.log(`deviceModel: ${options.deviceModel}`);
}
if (options.language) {
    console.log(`language: ${options.language}`);
}
if (options.output) {
    console.log(`output: ${options.output}`);
}

// import the json-source-map library
const jsonMap = require('json-source-map');

// import the path and fs libraries
const path = require('node:path');
const fs = require('fs');

// verify input file exists
if (!fs.existsSync(options.deviceModel)) {
    console.log(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}

// extract schema name from input filename
const info = path.parse(options.deviceModel).name.split('.');
const schemaName = info[0];
const namespace = info[1];

// read the schema definition file
const schemaFilename = '../../schema/catena.schema.json';
if (!fs.existsSync(schemaFilename)) {
    console.log(`Cannot open schema file at: ${schemaFilename}`);
    process.exit(1);
}
let schema = JSON.parse(fs.readFileSync(schemaFilename));

// let the user know what we're doing
console.log(`applying: ${schemaName} schema to input file ${options.deviceModel}`);

// adds schemas to the avj engine
function addSchemas(genus) {
    let schemaMap = jsonMap.stringify(schema, null, 2)

    for (species in schema[genus]) {
        if (species.indexOf('$comment') === 0) {
            // ignore comments
        } else {
            // treat as a schema
            try {
                ajv.addSchema(schema[genus][species], `#/${genus}/${species}`);
            } catch (why) {
                let errorPointer = schemaMap.pointers[`/${genus}/${species}`];

                throw Error(`${why} at #/${genus}/${species} on lines ${errorPointer.value.line}-${
                  errorPointer.valueEnd.line}`)
            }
        }
    }
}

// show errors
function showErrors(errors, sourceMap) {
    for (const err of errors) {
        switch (err.keyword) {
            case "maximum":
                console.log(err.limit);
                break;
            default:
                let errorPointer = sourceMap.pointers[err.instancePath];
                console.log(`${err.message} at ${err.instancePath} on lines ${errorPointer.value.line}-${
                  errorPointer.valueEnd.line}`);
                break;
        }
    }
}

const kDeviceSchema = schemaName.indexOf('device') === 0;

try {
    // the top-level schemas are under $defs, so add them
    addSchemas('$defs');
    if (!kDeviceSchema && !(schemaName in schema.$defs)) {
        throw {error: 2, message: `Could not find ${schemaName} in schema definition file.`};
    }

    // read the file to validate
    const data = JSON.parse(fs.readFileSync(options.deviceModel));
    const map = jsonMap.stringify(data, null, 2);
    let valid = false;

    if (kDeviceSchema) {
        const validate = ajv.compile(schema);
        if (validate(data)) {
            console.log('Device model is valid.');
            valid = true;
        } else {
            showErrors(validate.errors, map);
        }
    } else {
        if (ajv.validate(schema.$defs[schemaName], data)) {
            console.log(`Input was valid against the ${schemaName} schema.`);
            valid = true;
        } else {
            showErrors(ajv.errors, map);
        }
    }


    if (valid && kDeviceSchema) {
        // load the code generator
        const codegen = require(`./${options.language}gen.js`);
        codegen.setNamespace(namespace);
        for (p in data.params) {
            codegen.convert(p, data.params[p]);
        }
    }

} catch (why) {
    console.log(why.message);
    process.exit(why.error);
}
