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

import { After, setWorldConstructor, World } from '@cucumber/cucumber';
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

/** Directory holding the .feature files and their fixtures. */
export const FEATURES_DIR = path.resolve(__dirname, '..');
/** Root of the codegen tool (where codegen.js lives). */
export const CODEGEN_ROOT = path.resolve(FEATURES_DIR, '..');
export const CODEGEN_JS = path.join(CODEGEN_ROOT, 'codegen.js');
export const MODELS_DIR = path.join(FEATURES_DIR, 'fixtures', 'models');
export const GOLDEN_DIR = path.join(FEATURES_DIR, 'fixtures', 'golden');

/** File extensions emitted per target language. */
export const LANGUAGE_EXTENSIONS = {
  cpp: ['h', 'cpp'],
  go: ['go'],
};

/** Set UPDATE_GOLDENS=1 to (re)write golden files instead of asserting. */
export const UPDATE_GOLDENS = /^(1|true|yes)$/i.test(process.env.UPDATE_GOLDENS ?? '');

/**
 * The extensionless suffix codegen appends to output files is the model's
 * basename, e.g. `device.hello_world.yaml` -> `device.hello_world.yaml.h`.
 */
function outputBase(modelFilename) {
  return modelFilename;
}

class CodegenWorld extends World {
  constructor(options) {
    super(options);
    /** @type {string|null} basename of the selected fixture model */
    this.model = null;
    /** @type {Array<object>} one entry per codegen invocation */
    this.runs = [];
    this.workDir = fs.mkdtempSync(path.join(os.tmpdir(), 'codegen-e2e-'));
  }

  modelPath() {
    if (!this.model) {
      throw new Error('No device model selected; use the "Given" step first.');
    }
    return path.join(MODELS_DIR, this.model);
  }

  /**
   * Invoke codegen.js as a subprocess and capture its output files.
   * @param {string} language target language (e.g. "cpp")
   * @returns {object} run record { language, exitCode, stdout, stderr, outDir, files }
   */
  runCodegen(language) {
    const outDir = fs.mkdtempSync(path.join(this.workDir, `${language}-`));
    const result = spawnSync(
      process.execPath,
      [CODEGEN_JS, '--quiet', language, this.modelPath(), '--output', outDir],
      { encoding: 'utf8' }
    );

    const files = {};
    const base = outputBase(this.model);
    const extensions = LANGUAGE_EXTENSIONS[language] ?? [];
    for (const ext of extensions) {
      const filePath = path.join(outDir, `${base}.${ext}`);
      if (fs.existsSync(filePath)) {
        files[`${base}.${ext}`] = fs.readFileSync(filePath, 'utf8');
      }
    }

    const run = {
      language,
      exitCode: result.status,
      stdout: result.stdout ?? '',
      stderr: result.stderr ?? '',
      outDir,
      files,
    };
    this.runs.push(run);
    return run;
  }

  lastRun() {
    if (this.runs.length === 0) {
      throw new Error('No codegen run recorded; use a "When" step first.');
    }
    return this.runs[this.runs.length - 1];
  }

  /** Header file content from the given run (first extension for the language). */
  headerContent(run = this.lastRun()) {
    const [headerExt] = LANGUAGE_EXTENSIONS[run.language] ?? [];
    return run.files[`${outputBase(this.model)}.${headerExt}`];
  }

  /** Absolute path to a golden file for the current model/language. */
  goldenPath(language, ext) {
    return path.join(GOLDEN_DIR, language, `${outputBase(this.model)}.${ext}`);
  }
}

setWorldConstructor(CodegenWorld);

After(function () {
  if (this.workDir && fs.existsSync(this.workDir)) {
    fs.rmSync(this.workDir, { recursive: true, force: true });
  }
});
