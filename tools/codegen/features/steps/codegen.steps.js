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

import { Given, Then, When } from '@cucumber/cucumber';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {
  GOLDEN_DIR,
  LANGUAGE_EXTENSIONS,
  MODELS_DIR,
  UPDATE_GOLDENS,
} from '../support/world.js';

Given('the device model {string}', function (model) {
  const modelPath = path.join(MODELS_DIR, model);
  assert.ok(
    fs.existsSync(modelPath),
    `Fixture device model not found: ${modelPath}`
  );
  this.model = model;
});

Given('a nonexistent device model {string}', function (model) {
  this.model = model;
});

When('I run codegen for {string}', function (language) {
  this.runCodegen(language);
});

When('I run codegen for {string} again', function (language) {
  this.runCodegen(language);
});

When('I run codegen for {string} with {string}', function (language, extra) {
  this.runCodegen(language, { extraArgs: extra.split(/\s+/).filter(Boolean) });
});

When('I run codegen for {string} without --quiet', function (language) {
  this.runCodegen(language, { quiet: false });
});

When('I run codegen with {string}', function (args) {
  this.runArgs(args.split(/\s+/).filter(Boolean));
});

Then('the command exits successfully', function () {
  const run = this.lastRun();
  assert.equal(
    run.exitCode,
    0,
    `codegen exited with ${run.exitCode}\nstderr:\n${run.stderr}`
  );
});

Then('the command exits with a non-zero status', function () {
  const run = this.lastRun();
  assert.notEqual(
    run.exitCode,
    0,
    `Expected a non-zero exit code but codegen succeeded\nstdout:\n${run.stdout}`
  );
});

Then('no output files are written', function () {
  const run = this.lastRun();
  const written = Object.keys(run.files);
  assert.equal(
    written.length,
    0,
    `Expected no output files but codegen wrote: ${written.join(', ')}`
  );
});

Then('output files are written', function () {
  const run = this.lastRun();
  assert.ok(
    Object.keys(run.files).length > 0,
    'Expected codegen to write output files but none were found'
  );
});

Then('the output is not empty', function () {
  const run = this.lastRun();
  assert.ok(
    run.stdout.trim().length > 0,
    'Expected progress output on stdout but it was empty'
  );
});

Then('nothing is printed to standard output', function () {
  const run = this.lastRun();
  assert.equal(
    run.stdout.trim(),
    '',
    `Expected empty stdout but got:\n${run.stdout}`
  );
});

Then('the output matches a semantic version', function () {
  const run = this.lastRun();
  assert.match(
    run.stdout.trim(),
    /^\d+\.\d+\.\d+/,
    `Expected a semantic version on stdout but got: ${run.stdout.trim()}`
  );
});

Then('the generated files match the golden {string} output', function (language) {
  const run = this.lastRun();
  const extensions = LANGUAGE_EXTENSIONS[language] ?? [];
  assert.ok(extensions.length > 0, `Unknown language: ${language}`);

  for (const ext of extensions) {
    const name = `${this.model}.${ext}`;
    const actual = run.files[name];
    assert.ok(actual !== undefined, `codegen did not emit ${name}`);

    const goldenFile = this.goldenPath(language, ext);
    if (UPDATE_GOLDENS) {
      fs.mkdirSync(path.dirname(goldenFile), { recursive: true });
      fs.writeFileSync(goldenFile, actual);
      continue;
    }

    assert.ok(
      fs.existsSync(goldenFile),
      `Missing golden file: ${path.relative(GOLDEN_DIR, goldenFile)} ` +
        `(run with UPDATE_GOLDENS=1 to create it)`
    );
    const expected = fs.readFileSync(goldenFile, 'utf8');
    assert.equal(
      actual,
      expected,
      `Generated ${name} differs from golden ${path.relative(GOLDEN_DIR, goldenFile)}`
    );
  }
});

Then('both runs produce identical output', function () {
  assert.ok(this.runs.length >= 2, 'Expected at least two codegen runs');
  const [first, second] = this.runs.slice(-2);
  assert.deepEqual(
    second.files,
    first.files,
    'Repeated codegen runs produced different output'
  );
});

Then('the generated header contains {string}', function (needle) {
  const header = this.headerContent();
  assert.ok(header !== undefined, 'No header file was generated');
  assert.ok(
    header.includes(needle),
    `Generated header does not contain: ${needle}`
  );
});

Then('the generated header is guarded with {string}', function (guard) {
  const header = this.headerContent();
  assert.ok(header !== undefined, 'No header file was generated');
  assert.ok(
    header.includes(guard),
    `Generated header is not guarded with: ${guard}`
  );
});