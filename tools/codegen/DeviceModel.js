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

import { basename } from 'path';
import { descriptorIdFromUrl, formatDiagnostic, resolve } from 'smpte-validator';
import { ExitError } from './common.js';

/**
 * @typedef {Object} DeviceModel
 * @property {string} baseFilename
 * @property {string} deviceName
 * @property {object} desc   // resolved tree (ResolutionResult.data)
 */

/**
 * Call the smpte resolver to load the path into a DeviceModel
 * @param {string} url the path to load
 * @param {function(...any): void} log logging function (e.g., console.log)
 * @param {object} options resolution options
 * @param {boolean} options.disableMandatoryEnforcement whether to disable mandatory parameter enforcement
 * @return {Promise<DeviceModel>}
 */
export async function resolveDeviceModel(url, log, options) {
    // resolve the device model
    log(`Resolving device model ${url}...`);
    // make sure its a device, resolve works on anything
    let descId;
    try {
        descId = descriptorIdFromUrl(url);
    } catch (err) {
        // if the url is malformed, it will throw an error which we catch here
        // convert into an ExitError for nice output
        throw new ExitError(err.message);
    }
    if (descId.kind !== 'device') {
        throw new ExitError(`File must be a device model, not ${descId.kind}`);
    }
    const resolveResults = await resolve(url, {
        disableMandatoryParams: options.disableMandatoryEnforcement,
        // st2138_sdk / st2138_sdk_version values are injected by the SDK
        // toolchain (see cppgen), so authors need not provide them.
        sdkSuppliedProductParams: ['st2138_sdk', 'st2138_sdk_version'],
    })

    if (!resolveResults.valid) {
        throw new ExitError(resolveResults.diagnostics.map(d => formatDiagnostic(d)).join('\n'));
    }
    return { baseFilename: basename(url), deviceName: descId.name, desc: resolveResults.data };
}
