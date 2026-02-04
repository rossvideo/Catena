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

import fs from 'fs/promises';
import path from 'path';
import yaml from 'yaml';
import { program } from 'commander';
import { createLogger, OUTPUT_OPTION, PROTOS_OPTION, QUIET_OPTION, sync, VERSION } from './common.js';
import { deserialize } from './serdes/serdes.js';

//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
program
    .description("Deserialize binary to device model")
    .version(VERSION)
    .option(...QUIET_OPTION)
    .option(...OUTPUT_OPTION)
    .option(...PROTOS_OPTION)
    .option("--metadata", "Just output metadata", false)
    .option("--yaml", "Output device model in YAML format (default)", false)
    .option("--json", "Output device model in JSON format", false)
    .argument("<input>", "Input binary file")
    .action(run);

/**
 * log the options being used
 * @param {function} log logging function
 * @param {string} input path to binary input file
 * @param {string} format output format
 * @param {object} options options from commander
 */
function logOptions(log, input, format, options) {
    log(`input: ${input}`);
    log(`format: ${format}`);
    if (options.output) {
        log(`output: ${options.output}`);
    }
    if (options.protos) {
        log(`protos: ${options.protos}`);
    }
    if (options.metadata) {
        log(`Outputting metadata only`);
    }
}

// run the command line parser
sync(program);

async function run(input, options) {
    // determine output format
    const format = options.json && !options.yaml ? "json" : "yaml";

    // log stuff
    const log = createLogger(options);
    logOptions(log, input, format, options);

    log(`Deserializing binary file ${input}...`);
    const [metadata, deviceModel] = await deserialize({
        input,
        ...options,
    });
    if (options.metadata) {
        // don't use log, always output metadata, if requested
        console.log("Metadata:");
        console.log(JSON.stringify(metadata, null, 2));
        return;
    }
    log("Writing deserialized device model to output...");
    let output = "";
    let outputPath = options.output;
    const inputFileName = path.basename(input);
    const inputExt = path.extname(inputFileName) || /$/;
    outputPath = path.join(outputPath, inputFileName.replace(inputExt, `.${format}`));
    if (format === "json") {
        output = JSON.stringify(deviceModel, null, 4);
    } else {
        output = yaml.stringify(deviceModel);
    }
    await fs.mkdir(path.dirname(outputPath), { recursive: true });
    await fs.writeFile(outputPath, output, "utf-8");
    log(`Wrote deserialized device model to ${outputPath}`);
}
