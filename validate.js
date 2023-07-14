// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//
// Validates .json files against Catena schemata defined in
// ./schema/catena.schema.json
//

const Ajv = require('ajv');
const ajv = new Ajv({strict: false});  // our use of "strict" as a schema interferes with ajv's strict mode.

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
    console.log('Cannot open file at ${testfile}');
    process.exit(1);
}

// extract schema name from input filename
const schemaName = path.parse(testfile).name.split('.')[0];

// read the schema definition file
const schemaFilename = './schema/catena.schema.json';
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
