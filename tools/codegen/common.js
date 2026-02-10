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

import packageJson from './package.json' with { type: "json" };

/** @typedef {import('commander').Command} Command */

export const QUIET_OPTION = ["-q, --quiet", "Suppress non-error console output", false];
export const MANDATORY_OPTION = ["--disable-mandatory-enforcement", "Disable enforcement of mandatory parameters during code generation"];
export const OUTPUT_OPTION = ["-o --output <string>", "Output folder for results", "."];
export const PROTOS_OPTION = ["--protos <string>", "path to protobuf definitions", "../../smpte/interface/proto"];
export const VERSION = packageJson.version;

/**
 * Create a logging function based on options.
 * @param {object} options options from commander
 * @param {boolean} options.quiet suppress console output
 * @returns {function} logging function
 */
export function createLogger(options) {
    const quiet = options.quiet || false;
    return quiet ? () => { } : console.log;
}

/**
 * helper to run the commander program asynchronously
 * @param {Command} program the commander program
 */
export function sync(program) {
    (async () => {
        await program.parseAsync();
    })().catch((err) => {
        console.error("Error:", err);
        process.exit(1);
    });
}
