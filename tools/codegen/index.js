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


//
// Converts Catena compatible Device Models to computer code in a variety of languages
//

// load the command line parser
const { program } = require('commander');
program
    .option('-s, --schema <string>', 'path to schema definitions', '../../schema')
    .option('-d, --device-model <string>', 'Catena device model to process', '../../example_device_models/device.minimal.json')
    .option('-l, --language <string>', 'Language to generate code for', 'cpp')
    .option('-o, --output <string>', 'Output folder for generated code', '.');

program.parse(process.argv);
const options = program.opts();
if (options.schema) {
    console.log(`schema: ${options.schema}`);
}
if (options.deviceModel) {
    console.log(`deviceModel: ${options.deviceModel}`);
}
if (options.language) {
    console.log(`language: ${options.language}`);
}
if (options.output) {
    console.log(`output: ${options.output}`);
}




// import the fs libraries
const fs = require('fs');

// verify input file exists
if (!fs.existsSync(options.deviceModel)) {
    console.log(`Cannot open file at ${options.deviceModel}`);
    process.exit(1);
}


// read the schema definition file
const schemaFilename = options.schema;

const Validator = require('./validator.js');

const validator = new Validator(schemaFilename);

try {
 
    // read the file to validate
    const data = JSON.parse(fs.readFileSync(options.deviceModel));
    if (validator.validateDevice(data)) {
        const CodeGen =  require(`./${options.language}/${options.language}gen.js`);
        const codeGen = new CodeGen(options.deviceModel, options.output, validator, data);
        codeGen.generate();
    }

} catch (why) {
    console.log(why.message);
    process.exit(typeof why.error === 'number' ? why.error : 1);
}
