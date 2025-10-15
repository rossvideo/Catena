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

import { existsSync, readFileSync } from 'fs';
import { basename, dirname, extname } from "path";
import yaml from 'yaml';

import Validator from 'smpte-validator';

// TODO get the max depth from the smpte spec
const MAX_RESOLVE_DEPTH = 100;

/**
 * @class DeviceModel
 * @brief Holds the information parsed from a json device model file
 */
export class DeviceModel {
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
        this.baseFilename = basename(filePath);
        const info = this.baseFilename.split(".");
        const schemaName = info[0];
        if (schemaName !== "device") {
            throw new Error(`File must be a device model, not ${schemaName}`);
        }
        this.deviceName = info[1];
    }

    /**
     * Walk through the param and command hierarchy and resolve any imports. Note that
     * for simplicity in the schemata/protos, commands and params are both Param types.
     * @param {object} params the params or commands object to walk through
     * @todo this will change when smpte updates with a different param definition.
     * The top level params and commands objects will still exist, but sub-params
     * and sub-commands will be combined into a single 'typedef' object.
     */
    resolveImports() {
        const walk = (params, depth = 0) => {
            if (depth > MAX_RESOLVE_DEPTH) {
                throw new Error(`Max resolve depth exceeded`);
            }
            depth++;
            for (let [name, param] of Object.entries(params)) {
                if (param.import) {
                    param = this.importParam(param.import);
                    params[name] = param;
                }
                // recurse into sub-params and sub-commands
                // this is the part will change to typedefs when smpte updates
                if (param.params) {
                    walk(param.params);
                }
                if (param.commands) {
                    walk(param.commands);
                }
            }
        }
        // go through the top level params and commands
        if (this.desc.params) {
            walk(this.desc.params);
        }
        if (this.desc.commands) {
            walk(this.desc.commands);
        }
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
            const importDir = dirname(this.filePath);
            const importPath = `${importDir}/${importArg.file}`;
            if (!existsSync(importPath)) {
                throw new Error(`Cannot open file at ${importArg.file}`);
            }
            // Determining if the file is yaml or json and parsing accordingly.
            const importData = (() => {
                const extension = extname(importPath);
                if (extension === ".yaml" || extension === ".yml") {
                    return yaml.parse(readFileSync(importPath, 'utf8'));
                } else { // Default
                    return JSON.parse(readFileSync(importPath));
                }
            })();
            return importData;

        } else {
            throw new Error(`Unsupported import type: ${importArg}`);
        }
    }
}
