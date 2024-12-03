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
// Validates .json files against Catena schemata defined in
// ./schema/catena.device_schema.json
//

const Ajv = require('ajv');
<<<<<<< HEAD
const ajv = new Ajv({strict: false});  // our use of "strict" as a schema interferes with ajv's strict mode.
=======
const addFormats = require('ajv-formats').default;

const ajv = new Ajv({strict: false});  // our use of "strict" as a schema interferes with ajv's strict mode.
addFormats(ajv);
>>>>>>> develop

const jsonMap = require('json-source-map');

'use strict';
const path = require('node:path');
const fs = require('fs');


// get the file to validate from the command line
const testfile = process.argv[2];

// validate command line
if (testfile === undefined) {
    console.log('Usage: node app.js path/to/test/schema-name.object-name.json');
    console.log('Example: nodejs validate.js ./tests/device.my-device.json\nWill apply the device schema.');
    process.exit(1);
}

// verify input file exists
if (!fs.existsSync(testfile)) {
<<<<<<< HEAD
    console.log('Cannot open file at ${testfile}');
=======
    console.log(`Cannot open file at ${testfile}`);
>>>>>>> develop
    process.exit(1);
}

// extract schema name from input filename
const schemaName = path.parse(testfile).name.split('.')[0];

// read the schema definition file
const schemaFilename = '../../schema/catena.device_schema.json';
if (!fs.existsSync(schemaFilename)) {
    console.log(`Cannot open schema file at: ${schemaFilename}`);
    process.exit(1);
}
let schema = JSON.parse(fs.readFileSync(schemaFilename));

// let the user know what we're doing
console.log(`applying: ${schemaName} schema to input file ${testfile}`);


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
    const data = JSON.parse(fs.readFileSync(testfile));
    const map = jsonMap.stringify(data, null, 2);

    if (kDeviceSchema) {
        const validate = ajv.compile(schema);
        if (validate(data)) {
            console.log('Device model is valid.');
        } else {
            showErrors(validate.errors, map);
        }
    } else {
        if (ajv.validate(schema.$defs[schemaName], data)) {
            console.log('data was valid.');
        } else {
            showErrors(ajv.errors, map);
        }
    }

} catch (why) {
    console.log(why.message);
    process.exit(why.error);
}
