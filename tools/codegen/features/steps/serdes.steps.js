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

import { Then, When } from '@cucumber/cucumber';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import yaml from 'yaml';
import { resolveDeviceModel } from '../../DeviceModel.js';

/**
 * Assert that every field present in `subset` also appears, with the same
 * value, in `superset`. Deserialization turns implied defaults from the input
 * into concrete present fields, so the output is a superset of the input.
 */
function assertSuperset(subset, superset, at = '<root>') {
  if (Array.isArray(subset)) {
    assert.ok(Array.isArray(superset), `${at}: expected an array`);
    assert.ok(
      superset.length >= subset.length,
      `${at}: array has ${superset.length} entries, fewer than the input's ${subset.length}`
    );
    subset.forEach((value, index) =>
      assertSuperset(value, superset[index], `${at}[${index}]`)
    );
  } else if (subset !== null && typeof subset === 'object') {
    assert.ok(
      superset !== null && typeof superset === 'object' && !Array.isArray(superset),
      `${at}: expected an object`
    );
    for (const key of Object.keys(subset)) {
      assert.ok(
        Object.prototype.hasOwnProperty.call(superset, key),
        `${at}.${key}: missing from the deserialized model`
      );
      assertSuperset(subset[key], superset[key], `${at}.${key}`);
    }
  } else if (typeof subset === 'number' && typeof superset === 'number') {
    // float32 fields lose precision on the wire, so accept the 32-bit rounding.
    assert.ok(
      superset === subset || Math.fround(subset) === superset,
      `${at}: value ${superset} differs from the input ${subset}`
    );
  } else {
    assert.deepEqual(superset, subset, `${at}: value differs from the input`);
  }
}

When('I run ser with {string}', function (args) {
  this.runToolArgs('ser', args.split(/\s+/).filter(Boolean));
});

When('I run des with {string}', function (args) {
  this.runToolArgs('des', args.split(/\s+/).filter(Boolean));
});

When('I serialize the model', function () {
  this.runSer(this.modelPath());
});

When('I serialize the model without --quiet', function () {
  this.runSer(this.modelPath(), { quiet: false });
});

When('I deserialize the binary', function () {
  assert.ok(this.lastBin, 'No serialized binary available; serialize first.');
  this.runDes(this.lastBin);
});

When('I deserialize the binary as {string}', function (format) {
  assert.ok(this.lastBin, 'No serialized binary available; serialize first.');
  this.runDes(this.lastBin, { format });
});

When('I deserialize the binary showing only metadata', function () {
  assert.ok(this.lastBin, 'No serialized binary available; serialize first.');
  this.runDes(this.lastBin, { metadata: true });
});

When('I deserialize the model file directly', function () {
  this.runDes(this.modelPath());
});

Then('a binary artifact is produced', function () {
  const run = this.lastRun();
  assert.ok(run.binPath, 'ser did not write a binary artifact');
  const stats = fs.statSync(run.binPath);
  assert.ok(stats.size > 0, 'Serialized binary is empty');
});

Then('the deserialized output is {string}', function (format) {
  const run = this.lastRun();
  assert.ok(run.outPath, 'des did not write a model file');
  assert.ok(
    run.outPath.endsWith(`.${format}`),
    `Expected a .${format} file but got ${run.outPath}`
  );
  const text = fs.readFileSync(run.outPath, 'utf8');
  const parsed = format === 'json' ? JSON.parse(text) : yaml.parse(text);
  assert.ok(
    parsed && typeof parsed === 'object',
    'Deserialized output did not parse into a model object'
  );
});

Then('the metadata is printed', function () {
  const run = this.lastRun();
  assert.match(run.stdout, /Metadata:/, 'Expected metadata header on stdout');
  const json = run.stdout.slice(run.stdout.indexOf('{'));
  const metadata = JSON.parse(json);
  assert.ok(metadata.payload, 'Metadata is missing the payload descriptor');
});

Then('the deserialized model is a superset of the input model', async function () {
  assert.ok(this.lastModel, 'No deserialized model available; deserialize first.');
  const output = yaml.parse(fs.readFileSync(this.lastModel, 'utf8'));
  const input = await resolveDeviceModel(this.modelPath(), () => {}, {});
  assertSuperset(input.desc, output);
});

Then('the deserialized model still validates against the schema', function () {
  assert.ok(this.lastModel, 'No deserialized model available; deserialize first.');
  const run = this.runSer(this.lastModel);
  assert.equal(
    run.exitCode,
    0,
    `Deserialized model failed schema validation\nstderr:\n${run.stderr}`
  );
});
