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
    constructor(schemaFilename) {
        if (!fs.existsSync(schemaFilename)) {
            // bail if we cannot open the schema definition file
            throw new Error(`Cannot open schema file at: ${schemaFilename}`);
        }

        // read the schema definition file
        this.schema = JSON.parse(fs.readFileSync(schemaFilename, 'utf8'));

        ajv.addSchema(this.schema, '#');
        // add the subschemas to the avj engine
        this.schemaMap = jsonMap.stringify(this.schema, null, 2)
        for (const subschema in this.schema.$defs) {
            if (!subschema.indexOf('$comment') === 0) {
                try {
                    ajv.addSchema(this.schema.$defs[subschema], `#/$defs/${subschema}`);
                } catch (why) {
                    let errorPointer = this.schemaMap.pointers[`/$defs/${subschema}`];
                    throw Error(`${why} at #/$defs/${subschema} on lines ${errorPointer.value.line}-${
                    errorPointer.valueEnd.line}`)
                }
            }
        }
        
    }

    /**
     * validateData
     * @param {*} data 
     * @returns boolean, true if the data is valid, false otherwise
     */
    validate(schemaName, data) {
        let valid = false;
        const schema = schemaName === 'device' ? this.schema : this.schema.$defs[schemaName];
        if (ajv.validate(schema, data)) {
            console.log(`Input was valid against the ${schemaName} schema.`);
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