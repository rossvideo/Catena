import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { expect, test, describe, beforeAll } from '@jest/globals';
import { getCppIdentifier } from '../cpp/param.js';
import CppGen from '../cpp/cppgen.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUTPUT_DIR = path.join(__dirname, 'output', 'cpp-keywords-integration');

/**
 * Minimal valid device descriptor with C++ keywords as param oids:
 * - "auto": top-level param and struct field (inputs.auto)
 * - "class": top-level param
 * - "switch": struct field (inputs.switch)
 */
const MINIMAL_DESCRIPTOR_WITH_KEYWORDS = {
  slot: 1,
  detail_level: 'FULL',
  access_scopes: ['st2138:mon', 'st2138:op'],
  default_scope: 'st2138:op',
  params: {
    product: {
      type: 'STRUCT',
      access_scope: 'st2138:mon',
      read_only: true,
      value: {
        struct_value: {
          fields: {
            name: { string_value: 'Keyword Test' },
            vendor: { string_value: 'Test' },
            version: { string_value: '1.0.0' },
            catena_sdk: { string_value: 'https://example.com' },
            serial_number: { string_value: 'SN-001' }
          }
        }
      },
      params: {
        name: { type: 'STRING' },
        vendor: { type: 'STRING' },
        version: { type: 'STRING' },
        catena_sdk: { type: 'STRING' },
        catena_sdk_version: { type: 'STRING' },
        serial_number: { type: 'STRING' }
      }
    },
    auto: {
      type: 'STRING',
      name: { display_strings: { en: 'Auto Configure' } }
    },
    class: {
      type: 'STRING',
      name: { display_strings: { en: 'Class Name' } }
    },
    inputs: {
      type: 'STRUCT',
      name: { display_strings: { en: 'Inputs' } },
      params: {
        auto: {
          type: 'STRING',
          name: { display_strings: { en: 'Auto' } }
        },
        switch: {
          type: 'STRING',
          name: { display_strings: { en: 'Switch' } }
        }
      }
    }
  }
};

describe('getCppIdentifier', () => {
  test('returns safe identifier for C++ keyword "auto"', () => {
    expect(getCppIdentifier('auto')).toBe('auto_');
  });

  test('returns safe identifier for C++ keyword "class"', () => {
    expect(getCppIdentifier('class')).toBe('class_');
  });

  test('returns safe identifier for C++ keyword "private"', () => {
    expect(getCppIdentifier('private')).toBe('private_');
  });

  test('returns safe identifier for C++ keyword "public"', () => {
    expect(getCppIdentifier('public')).toBe('public_');
  });

  test('returns safe identifier for C++ keyword "protected"', () => {
    expect(getCppIdentifier('protected')).toBe('protected_');
  });

  test('returns safe identifier for C++ keyword "switch"', () => {
    expect(getCppIdentifier('switch')).toBe('switch_');
  });

  test('returns safe identifier for C++ keyword "if"', () => {
    expect(getCppIdentifier('if')).toBe('if_');
  });

  test('returns oid unchanged for non-keyword "video_file_path"', () => {
    expect(getCppIdentifier('video_file_path')).toBe('video_file_path');
  });

  test('returns oid unchanged for non-keyword "inputs"', () => {
    expect(getCppIdentifier('inputs')).toBe('inputs');
  });

  test('returns oid unchanged for non-keyword "product"', () => {
    expect(getCppIdentifier('product')).toBe('product');
  });

  test('returns oid unchanged for non-keyword "domain"', () => {
    expect(getCppIdentifier('domain')).toBe('domain');
  });

  test('returns oid unchanged for non-keyword "flow"', () => {
    expect(getCppIdentifier('flow')).toBe('flow');
  });
});

describe('C++ codegen keyword safeguarding', () => {
  const deviceModel = {
    baseFilename: 'device.keywords.json',
    deviceName: 'keywords',
    desc: MINIMAL_DESCRIPTOR_WITH_KEYWORDS
  };

  beforeAll(() => {
    fs.mkdirSync(OUTPUT_DIR, { recursive: true });
  });

  test('generated code uses safe identifiers and compilable C++', () => {
    const cppGen = new CppGen(deviceModel, OUTPUT_DIR);
    cppGen.generate();

    const headerPath = path.join(OUTPUT_DIR, 'device.keywords.json.h');
    const bodyPath = path.join(OUTPUT_DIR, 'device.keywords.json.cpp');
    expect(fs.existsSync(headerPath)).toBe(true);
    expect(fs.existsSync(bodyPath)).toBe(true);

    const header = fs.readFileSync(headerPath, 'utf8');
    const body = fs.readFileSync(bodyPath, 'utf8');
    const combined = header + '\n' + body;

    // Must NOT contain bare keywords as C++ identifiers (would be invalid)
    expect(combined).not.toMatch(/\bstd::string\s+auto\b/);
    expect(combined).not.toMatch(/\bstd::string\s+class\b/);
    expect(combined).not.toMatch(/\bstd::string\s+switch\b/);
    expect(combined).not.toMatch(/\bint32_t\s+auto\b/);
    expect(combined).not.toMatch(/::\s*auto\s*[;,)]/);
    expect(combined).not.toMatch(/::\s*class\s*[;,)]/);
    expect(combined).not.toMatch(/::\s*switch\s*[;,)]/);
    expect(combined).not.toMatch(/namespace\s+_\s*auto\s*\{/);

    // Must contain the safe forms (keyword + underscore) where keywords were used as identifiers
    expect(combined).toMatch(/auto_/);
    expect(combined).toMatch(/class_/);
    expect(combined).toMatch(/switch_/);

    // API/descriptor must still use original oid strings for the protocol
    expect(body).toMatch(/"auto"/);
    expect(body).toMatch(/"class"/);
    expect(body).toMatch(/"switch"/);
  });
});
