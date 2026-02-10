/*Copyright 2026 Ross Video Ltd
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

import { program } from 'commander';

import { createLogger, MANDATORY_OPTION, OUTPUT_OPTION, PROTOS_OPTION, QUIET_OPTION, sync, VERSION } from './common.js';
import { DeviceModel } from './DeviceModel.js';
import Validator from 'smpte-validator';
import { validateRequiredParamsAndScopes } from './mandatory.js';
import { serialize } from './serdes/serdes.js';

// load the command line parser
program
    .description("Serialize Catena device models to a binary format")
    .version(VERSION)
    .option(...OUTPUT_OPTION)
    .option(...QUIET_OPTION)
    .option(...MANDATORY_OPTION)
    .option(...PROTOS_OPTION)
    .argument("<deviceModel>", "Catena device model to process")
    .action(run);

/**
 * log the options being used
 * @param {function} log logging function
 * @param {string} deviceModel path to device model
 * @param {object} options options from commander
 */
function logOptions(log, deviceModel, options) {
    log(`deviceModel: ${deviceModel}`);
    if (options.output) {
        log(`output: ${options.output}`);
    }
    if (options.disableMandatoryEnforcement) {
        log(`Mandatory parameter enforcement disabled`);
    }
    if (options.protos) {
        log(`protos: ${options.protos}`);
    }
}

// run the command line parser
sync(program);

async function run(deviceModelPath, options) {
    // define logging function based on quiet option
    const log = createLogger(options);

    // log the options being used
    logOptions(log, deviceModelPath, options);

    // load and validate the device model
    const validator = new Validator();
    const deviceModel = new DeviceModel(deviceModelPath, validator);
    log(`Validating device model ${deviceModelPath}...`);
    await deviceModel.load(true);
    validateRequiredParamsAndScopes(deviceModel.desc, options.disableMandatoryEnforcement);

    // prepare metadata
    const metadata = {
        tool: "ser",
        version: VERSION,
        timestamp: new Date().toISOString(),
    };

    log("Generating serialized output...");
    await serialize(deviceModel, options, metadata);
    log('✅ Completed.');
}
