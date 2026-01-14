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

import { basename, resolve } from 'path';
import Validator from 'smpte-validator';

// TODO get the max depth from the smpte spec
const MAX_RESOLVE_DEPTH = 100;

export class DeviceModel {
    /**
     * Create a new DeviceModel instance. To load the model from file, call load().
     * @param {string} url the path to the device model file
     * @param {Validator} validator the validator to use
     */
    constructor(url, validator) {
        this.url = url;
        this.baseFilename = null;
        this.deviceName = null;
        this.validator = validator;
        this.desc = null;
    }

    /**
     * @brief Load the device model from file
     * @param {bool} resolveImports if true, resolve any imports in the model
     * @throws {Error} if the file is not a device model
     * @todo this will change when smpte updates with a different param definition.
     * The top level params and commands objects will still exist, but sub-params
     * and sub-commands will be combined into a single 'typedef' object.
     */
    async load(resolveImports) {
        this.baseFilename = basename(this.url);
        const info = this.baseFilename.split(".");
        const schemaName = info[0];
        if (schemaName !== "device") {
            throw new Error(`File must be a device model, not ${schemaName}`);
        }
        this.deviceName = info[1];
        if (this.url.indexOf("://") === -1) {
            this.url = new URL(resolve(this.url), "file://");
        } else {
            this.url = new URL(this.url);
        }
        this.desc = await this._load(schemaName, this.url, resolveImports, 0);
    }

    /**
     * Load a resource from a uri, validate it and return the parsed object.
     * @param {string} schemaName the schema name to validate against
     * @param {URL} url the uri to load
     * @param {bool} resolveImports if true, resolve any imports in the object
     * @param {number} depth the current recursion depth
     * @returns {Promise<object>} the parsed object
     * @throws {Error} if the resource cannot be loaded or is invalid
     */
    async _load(schemaName, url, resolveImports, depth) {
        const { valid, data } = await this.validator.validate(schemaName, url);
        if (!valid) {
            throw new Error(`Resource ${url} is invalid`);
        }
        if (resolveImports) {
            if (data.params) {
                await this._walk(data.params, depth + 1, url);
            }
            // istanbul ignore next, going away
            if (data.commands) {
                await this._walk(data.commands, depth + 1, url);
            }
        }
        return data;
    }

    /**
     * Walk the parameters and resolve any imports.
     * @param {object} params params to walk
     * @param {number} depth current recursion depth
     * @param {URL} currentUrl URL to put paths against
     */
    async _walk(params, depth, currentUrl) {
        if (depth > MAX_RESOLVE_DEPTH) {
            throw new Error("Maximum import depth exceeded");
        }
        for (let [name, param] of Object.entries(params)) {
            if (param.import) {
                const importUrl = new URL(param.import.url, currentUrl);
                if (!basename(importUrl.pathname).startsWith("param.")) {
                    throw new Error(`Imported file ${param.import.url} is not a param file`);
                }
                params[name] = param = await this._load("param", importUrl, true, depth);
            }
            // recurse into sub-params and sub-commands
            // this is the part will change to typedefs when smpte updates
            if (param.params) {
                await this._walk(param.params, depth + 1, currentUrl);
            }
            // istanbul ignore next, going away
            if (param.commands) {
                await this._walk(param.commands, depth + 1, currentUrl);
            }
        }
    }
}
