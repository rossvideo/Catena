import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { afterEach, describe, expect, test } from '@jest/globals';
import CppGen from '../../cpp/cppgen.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUTPUT_DIR = path.join(__dirname, '..', 'output', 'cppgen-test-output');

const SHARED_RANGE = {
  type: 'INT_RANGE',
  int32_range: { min_value: 0, max_value: 100 }
};

/** Every generate path needs menu_groups (menu() iterates it). */
function baseDesc(overrides = {}) {
  return {
    slot: 0,
    detail_level: 'FULL',
    access_scopes: ['st2138:mon', 'st2138:op'],
    default_scope: 'st2138:op',
    menu_groups: {},
    ...overrides
  };
}

function pathsForBaseFilename(baseFilename) {
  const headerPath = path.join(OUTPUT_DIR, `${baseFilename}.h`);
  const bodyPath = path.join(OUTPUT_DIR, `${baseFilename}.cpp`);
  return { headerPath, bodyPath };
}

function runGenerate(baseFilename, deviceName, desc) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
  const deviceModel = { baseFilename, deviceName, desc };
  const gen = new CppGen(deviceModel, OUTPUT_DIR);
  gen.generate();
  return pathsForBaseFilename(baseFilename);
}

describe('CppGen constructor', () => {
  test('sets headerFilename and builds Device from deviceModel', () => {
    fs.mkdirSync(OUTPUT_DIR, { recursive: true });
    const desc = baseDesc({ params: { leaf: { type: 'STRING' } } });
    const deviceModel = { baseFilename: 'ctor.json', deviceName: 'CtorNs', desc };
    const gen = new CppGen(deviceModel, OUTPUT_DIR);
    expect(gen.headerFilename).toBe('ctor.json.h');
    expect(gen.device.namespace).toBe('CtorNs');
    expect(gen.device.desc).toBe(desc);
    gen.finish();
    fs.closeSync(fs.openSync(path.join(OUTPUT_DIR, 'ctor.json.h'), 'r'));
    fs.closeSync(fs.openSync(path.join(OUTPUT_DIR, 'ctor.json.cpp'), 'r'));
  });
});

describe('CppGen.generate', () => {
  test('writes header and body with namespace and device initializer', () => {
    const baseFilename = 'minimal.json';
    const { headerPath, bodyPath } = runGenerate(
      baseFilename,
      'MinimalNs',
      baseDesc({
        params: {
          leaf: { type: 'STRING', name: { display_strings: { en: 'Leaf' } } }
        }
      })
    );
    expect(fs.existsSync(headerPath)).toBe(true);
    expect(fs.existsSync(bodyPath)).toBe(true);
    const header = fs.readFileSync(headerPath, 'utf8');
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(header).toContain('#pragma once');
    expect(header).toContain('namespace MinimalNs');
    expect(body).toContain('#include "minimal.json.h"');
    expect(body).toContain('catena::common::Device dm');
    expect(body).toContain('using namespace MinimalNs');
    expect(body).toContain('_leafDescriptor');
  });

  test('succeeds when desc has no params key', () => {
    const { headerPath, bodyPath } = runGenerate('noparams.json', 'NoP', baseDesc());
    expect(fs.readFileSync(bodyPath, 'utf8')).toContain('catena::common::Device dm');
    expect(fs.existsSync(headerPath)).toBe(true);
  });

  test('emits shared constraints from desc.constraints', () => {
    const { bodyPath } = runGenerate(
      'constraints.json',
      'C',
      baseDesc({
        constraints: { shared_range: SHARED_RANGE },
        params: {
          level: {
            type: 'INT32',
            constraint: { ref_oid: 'shared_range' }
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('RangeConstraint');
    expect(body).toContain('shared_range');
  });

  test('emits commands as ParamWithValue with is_command', () => {
    const { bodyPath } = runGenerate(
      'commands.json',
      'CmdDev',
      baseDesc({
        commands: {
          play: {
            type: 'EMPTY',
            name: { display_strings: { en: 'Play' } },
            response: false
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('_playParam');
    expect(body).toMatch(/ParamWithValue[\s\S]*true\)/);
  });

  test('command with STRUCT type and value writes type info and initializer', () => {
    const { headerPath, bodyPath } = runGenerate(
      'cmdstruct.json',
      'CmdStructNs',
      baseDesc({
        commands: {
          payload: {
            type: 'STRUCT',
            name: { display_strings: { en: 'Payload' } },
            response: false,
            value: {
              struct_value: {
                fields: {
                  x: { int32_value: 0 }
                }
              }
            },
            params: {
              x: { type: 'INT32' }
            }
          }
        }
      })
    );
    const header = fs.readFileSync(headerPath, 'utf8');
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(header).toContain('struct Payload');
    expect(body).toContain('Payload payload');
    expect(body).toContain('_payloadParam');
  });

  test('product param adds SDK version initialization to body coda', () => {
    const { bodyPath } = runGenerate(
      'product.json',
      'ProdNs',
      baseDesc({
        params: {
          product: {
            type: 'STRUCT',
            access_scope: 'st2138:mon',
            read_only: true,
            value: {
              struct_value: {
                fields: {
                  name: { string_value: 'T' },
                  vendor: { string_value: 'V' },
                  version: { string_value: '1' },
                  catena_sdk: { string_value: 'https://x' },
                  serial_number: { string_value: 'SN' },
                  catena_sdk_version: { string_value: '0' }
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
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('CATENA_CPP_VERSION');
    expect(body).toContain('initialize_sdk_version');
    expect(body).toContain('initialize_sdk_version(product)');
  });

  test('menu_groups produce MenuGroup and Menu in body', () => {
    const { bodyPath } = runGenerate(
      'menus.json',
      'MenuNs',
      baseDesc({
        params: {
          leaf: { type: 'STRING' }
        },
        menu_groups: {
          main: {
            name: { display_strings: { en: 'Main' } },
            menus: {
              settings: {
                name: { display_strings: { en: 'Settings' } },
                param_oids: ['leaf'],
                command_oids: [],
                client_hints: { hint: 'value' }
              }
            }
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('MenuGroup _mainGroup');
    expect(body).toContain('Menu _mainGroup_settingsMenu');
    expect(body).toContain('"leaf"');
    expect(body).toContain('{ "hint", "value" }');
  });

  test('language_packs produce LanguagePack in body', () => {
    const { bodyPath } = runGenerate(
      'lang.json',
      'LangNs',
      baseDesc({
        language_packs: {
          packs: {
            en: {
              name: 'English',
              words: { HELLO: 'Hello' }
            }
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('LanguagePack en');
    expect(body).toContain('"English"');
    expect(body).toContain('"HELLO"');
    expect(body).toContain('"Hello"');
  });

  test('STRUCT_VARIANT param writes variant and alternativeNames', () => {
    const { headerPath, bodyPath } = runGenerate(
      'variant.json',
      'VarNs',
      baseDesc({
        params: {
          choice: {
            type: 'STRUCT_VARIANT',
            name: { display_strings: { en: 'Choice' } },
            params: {
              a: { type: 'STRING' },
              b: { type: 'INT32' }
            }
          }
        }
      })
    );
    const header = fs.readFileSync(headerPath, 'utf8');
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(header).toContain('namespace _choice');
    expect(header).toContain('std::variant<');
    expect(header).toContain('alternativeNames');
  });

  test('STRUCT_ARRAY writes std::vector element alias in header', () => {
    const { headerPath } = runGenerate(
      'structarr.json',
      'ArrNs',
      baseDesc({
        params: {
          rows: {
            type: 'STRUCT_ARRAY',
            name: { display_strings: { en: 'Rows' } },
            params: {
              x: { type: 'INT32' }
            }
          }
        }
      })
    );
    const header = fs.readFileSync(headerPath, 'utf8');
    expect(header).toContain('struct Rows_elem');
    expect(header).toContain('std::vector<');
  });

  test('param with inline range constraint emits constraint initializer in body', () => {
    const { bodyPath } = runGenerate(
      'inlinerange.json',
      'RngNs',
      baseDesc({
        params: {
          gain: {
            type: 'FLOAT32',
            constraint: {
              type: 'FLOAT_RANGE',
              float32_range: {
                min_value: 0,
                max_value: 1,
                display_min: 0,
                display_max: 1
              }
            }
          }
        }
      })
    );
    const body = fs.readFileSync(bodyPath, 'utf8');
    expect(body).toContain('RangeConstraint');
    expect(body).toContain('gainConstraint');
  });

  test('nested STRUCT subparams recurse writeTypeInfo in header', () => {
    const { headerPath } = runGenerate(
      'nestedstruct.json',
      'NestNs',
      baseDesc({
        params: {
          outer: {
            type: 'STRUCT',
            params: {
              inner: {
                type: 'STRUCT',
                params: {
                  leaf: { type: 'STRING' }
                }
              }
            }
          }
        }
      })
    );
    const header = fs.readFileSync(headerPath, 'utf8');
    expect(header).toContain('struct Inner');
    expect(header).toContain('struct Outer');
  });

  test('STRUCT_VARIANT with STRUCT alternative recurses writeTypeInfo', () => {
    const { headerPath } = runGenerate(
      'varstruct.json',
      'VS',
      baseDesc({
        params: {
          pick: {
            type: 'STRUCT_VARIANT',
            params: {
              simple: { type: 'STRING' },
              nested: {
                type: 'STRUCT',
                params: { x: { type: 'INT32' } }
              }
            }
          }
        }
      })
    );
    const header = fs.readFileSync(headerPath, 'utf8');
    expect(header).toContain('namespace _pick');
    expect(header).toContain('struct Nested');
  });
});
