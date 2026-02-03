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

import packageJson from './package.json' with { type: "json" };;
import { DeviceModel } from './DeviceModel.js';
import Validator from 'smpte-validator';
import { validateRequiredParamsAndScopes } from './mandatory.js';
import CppGen from './cpp/cppgen.js';

//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
program
    .description(packageJson.description)
    .version(packageJson.version)
    .option('-q, --quiet', 'Suppress non-error console output', false)
    .option("--disable-mandatory-enforcement", "Disable enforcement of mandatory parameters during code generation")
    .option("-o --output <string>", "Output folder for results", ".")
    .argument("<language>", "Target programming language (cpp)")
    .argument("<deviceModel>", "Catena device model to process")
    .action(generate);

/**
 * log the options being used
 * @param {function} log logging function
 * @param {string} language target language
 * @param {string} deviceModel path to device model
 * @param {object} options options from commander
 */
function logOptions(log, language, deviceModel, options) {
    log(`deviceModel: ${deviceModel}`);
    log(`language: ${language}`);
    if (options.output) {
        log(`output: ${options.output}`);
    }
    if (options.disableMandatoryEnforcement) {
        log(`Mandatory parameter enforcement disabled`);
    }
}

// run the command line parser
(async () => {
    await program.parseAsync();
})().catch((err) => {
    console.error(`Error: ${err.message}`);
    process.exit(err.error || 1);
});

async function generate(language, deviceModelPath, options) {
    const log = options.quiet ? () => { } : console.log;
    logOptions(log, language, deviceModelPath, options);

    // load and validate the device model
    const validator = new Validator();
    const deviceModel = new DeviceModel(deviceModelPath, validator);
    log(`Validating device model ${deviceModelPath}...`);
    await deviceModel.load(true);
    validateRequiredParamsAndScopes(deviceModel.desc, options.disableMandatoryEnforcement);

    switch (language) {
        case 'cpp':
            log("Generating C++ code...");
            const cppGen = new CppGen(deviceModel, options.output);
            cppGen.generate();
            break;
        default:
            throw new Error(`Unsupported language: ${language}`);
    }
    log('✅ Code generation completed.');
}
