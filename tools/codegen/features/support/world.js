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
export const SER_JS = path.join(CODEGEN_ROOT, 'ser.js');
export const DES_JS = path.join(CODEGEN_ROOT, 'des.js');
export const PROTOS_DIR = path.resolve(CODEGEN_ROOT, '../../smpte/interface/proto');
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
   * @param {object} [opts]
   * @param {boolean} [opts.quiet=true] pass --quiet
   * @param {string[]} [opts.extraArgs=[]] extra flags placed before the language
   * @returns {object} run record { language, exitCode, stdout, stderr, outDir, files }
   */
  runCodegen(language, { quiet = true, extraArgs = [] } = {}) {
    const outDir = fs.mkdtempSync(path.join(this.workDir, `${language}-`));
    const args = [CODEGEN_JS];
    if (quiet) args.push('--quiet');
    args.push(...extraArgs, language, this.modelPath(), '--output', outDir);
    const result = spawnSync(process.execPath, args, { encoding: 'utf8' });

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

  /**
   * Invoke codegen.js with a raw argument list (no language/model/output added).
   * Used for CLI-level checks such as --version.
   * @param {string[]} args argv passed to codegen.js
   * @returns {object} run record with no captured files
   */
  runArgs(args) {
    const result = spawnSync(process.execPath, [CODEGEN_JS, ...args], {
      encoding: 'utf8',
    });
    const run = {
      language: null,
      exitCode: result.status,
      stdout: result.stdout ?? '',
      stderr: result.stderr ?? '',
      outDir: null,
      files: {},
    };
    this.runs.push(run);
    return run;
  }

  /**
   * Invoke ser.js on a device model and capture the produced binary.
   * @param {string} input path to the device model to serialize
   * @param {object} [opts]
   * @param {boolean} [opts.quiet=true] pass --quiet
   * @param {string[]} [opts.extraArgs=[]] extra flags placed before the model
   * @returns {object} run record { tool, exitCode, stdout, stderr, outDir, binPath }
   */
  runSer(input, { quiet = true, extraArgs = [] } = {}) {
    const outDir = fs.mkdtempSync(path.join(this.workDir, 'ser-'));
    const args = [SER_JS];
    if (quiet) args.push('--quiet');
    args.push('--protos', PROTOS_DIR, '--output', outDir, ...extraArgs, input);
    const result = spawnSync(process.execPath, args, { encoding: 'utf8' });

    let binPath = null;
    if (result.status === 0) {
      const produced = fs.readdirSync(outDir).filter((f) => f.endsWith('.bin'));
      if (produced.length > 0) binPath = path.join(outDir, produced[0]);
    }

    const run = {
      tool: 'ser',
      exitCode: result.status,
      stdout: result.stdout ?? '',
      stderr: result.stderr ?? '',
      outDir,
      binPath,
      files: {},
    };
    this.runs.push(run);
    if (binPath) this.lastBin = binPath;
    return run;
  }

  /**
   * Invoke des.js on a serialized binary and capture the produced model.
   * @param {string} input path to the binary to deserialize
   * @param {object} [opts]
   * @param {boolean} [opts.quiet=true] pass --quiet
   * @param {"yaml"|"json"} [opts.format="yaml"] output format
   * @param {boolean} [opts.metadata=false] pass --metadata (no model file emitted)
   * @param {string[]} [opts.extraArgs=[]] extra flags placed before the input
   * @returns {object} run record { tool, exitCode, stdout, stderr, outDir, outPath, format }
   */
  runDes(input, { quiet = true, format = 'yaml', metadata = false, extraArgs = [] } = {}) {
    const outDir = fs.mkdtempSync(path.join(this.workDir, 'des-'));
    const args = [DES_JS];
    if (quiet) args.push('--quiet');
    args.push('--protos', PROTOS_DIR, '--output', outDir);
    if (metadata) args.push('--metadata');
    else if (format === 'json') args.push('--json');
    else args.push('--yaml');
    args.push(...extraArgs, input);
    const result = spawnSync(process.execPath, args, { encoding: 'utf8' });

    let outPath = null;
    if (!metadata && result.status === 0) {
      const ext = format === 'json' ? '.json' : '.yaml';
      const produced = fs.readdirSync(outDir).filter((f) => f.endsWith(ext));
      if (produced.length > 0) outPath = path.join(outDir, produced[0]);
    }

    const run = {
      tool: 'des',
      exitCode: result.status,
      stdout: result.stdout ?? '',
      stderr: result.stderr ?? '',
      outDir,
      outPath,
      format,
      files: {},
    };
    this.runs.push(run);
    if (outPath) this.lastModel = outPath;
    return run;
  }

  /**
   * Invoke ser.js or des.js with a raw argument list (no defaults added).
   * Used for CLI-level checks such as --version.
   * @param {"ser"|"des"} tool which tool to invoke
   * @param {string[]} args argv passed to the tool
   * @returns {object} run record with no captured files
   */
  runToolArgs(tool, args) {
    const script = tool === 'ser' ? SER_JS : DES_JS;
    const result = spawnSync(process.execPath, [script, ...args], {
      encoding: 'utf8',
    });
    const run = {
      tool,
      exitCode: result.status,
      stdout: result.stdout ?? '',
      stderr: result.stderr ?? '',
      outDir: null,
      files: {},
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
