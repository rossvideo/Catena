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


/**
 * Validates input against the Catena json-schema.
 */

// load the validation engine
const Ajv = require('ajv');
const addFormats = require('ajv-formats').default;
const ajv = new Ajv({strict: false});  
addFormats(ajv);

// our use of "strict" as a schema interferes with ajv's strict mode.
// so we only turn it on after loading ajv
'use strict';

// import the json-source-map library
// import the path and fs libraries
const path = require('node:path');
const fs = require('fs');
const jsonMap = require('json-source-map');

class Validator {
    constructor(schemaDir) {
        let deviceSchemaFile = path.join(schemaDir, 'catena.device_schema.json');
        let paramSchemaFile = path.join(schemaDir, 'catena.param_schema.json');
        if (!fs.existsSync(deviceSchemaFile)) {
            // bail if we cannot open the schema definition file
            throw new Error(`Cannot open device schema file at: ${deviceSchemaFile}`);
        }
        if (!fs.existsSync(paramSchemaFile)) {
            // bail if we cannot open the schema definition file
            throw new Error(`Cannot open parameter schema file at: ${paramSchemaFile}`);
        }

        // read the schema definition file
        this.deviceSchema = JSON.parse(fs.readFileSync(deviceSchemaFile, 'utf8'));
        this.paramSchema = JSON.parse(fs.readFileSync(paramSchemaFile, 'utf8'));

        ajv.addSchema(this.deviceSchema);
        ajv.addSchema(this.paramSchema);
    }

    /**
     * validateData
     * @param {*} data 
     * @returns boolean, true if the data is valid, false otherwise
     */
    validateDevice(data) {
        let valid = false;
        const schema = this.deviceSchema;
        if (ajv.validate(schema, data)) {
            console.log(`Input was valid against the device schema.`);
            valid = true;
        } else {
            this.showErrors();
        }
        return valid;
    }

    validateParam(data) {
        let valid = false;
        const schema = this.paramSchema;
        if (ajv.validate(schema, data)) {
            console.log(`Input was valid against the parameter schema.`);
            valid = true;
        } else {
            this.showErrors();
        }
        return valid;
    }

    /**
     * showErrors
     * Displays the validation errors
     */
    showErrors() {
        if (!ajv.errors) {
            console.log('No errors found.');
            return;
        }
        for (const err of ajv.errors) {
            switch (err.keyword) {
                case "maximum":
                    console.log(err.limit);
                    break;
                default:
                    console.log(`${err.message} at ${err.instancePath}`);
                    break;
            }
        }
    }
}

module.exports = Validator;