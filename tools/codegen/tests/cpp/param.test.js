import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { expect, test, describe, beforeAll } from '@jest/globals';
import Param, { getCppIdentifier } from '../../cpp/param.js';
import { CPP_KEYWORDS } from '../../cpp/cpp-keywords.js';
import CppGen from '../../cpp/cppgen.js';
import Device from '../../cpp/device.js';
import Constraint from '../../cpp/constraint.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUTPUT_DIR = path.join(__dirname, '..', 'output', 'cpp-test-output');

/** Shared constraint for ref_oid coverage */
const SHARED_RANGE_CONSTRAINT = {
  type: 'INT_RANGE',
  int32_range: { min_value: 0, max_value: 100 }
};

/**
 * Minimal valid device descriptor with C++ keywords as param oids:
 * - "auto": top-level param and struct field (inputs.auto)
 * - "class": top-level param
 * - "switch": struct field (inputs.switch)
 * Plus constraint (ref_oid + inline), template_oid, and minimal_set inheritance.
 */
const MINIMAL_DESCRIPTOR_WITH_KEYWORDS = {
  slot: 1,
  detail_level: 'FULL',
  access_scopes: ['st2138:mon', 'st2138:op'],
  default_scope: 'st2138:op',
  constraints: {
    shared_range: SHARED_RANGE_CONSTRAINT
  },
  params: {
    item: {
      type: 'STRUCT',
      name: { display_strings: { en: 'Item' } },
      params: { sub: { type: 'STRING' } },
      minimal_set: true
    },
    item_list: {
      type: 'STRUCT_ARRAY',
      name: { display_strings: { en: 'Item List' } },
      template_oid: '/item'
    },
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
        struct: { type: 'STRUCT', params: {nested: { type: 'STRING' }}},
        arr: { type: 'STRING_ARRAY' }
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
    },
    level: {
      type: 'INT32',
      name: { display_strings: { en: 'Level' } },
      constraint: { ref_oid: 'shared_range' }
    },
    volume: {
      type: 'INT32',
      name: { display_strings: { en: 'Volume' } },
      constraint: {
        type: 'INT_RANGE',
        int32_range: { min_value: 0, max_value: 10 }
      }
    }
  }
};

describe('getCppIdentifier', () => {
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

  test('every CPP_KEYWORDS entry returns safe identifier (keyword + underscore)', () => {
    for (const keyword of CPP_KEYWORDS) {
      expect(getCppIdentifier(keyword)).toBe(`${keyword}_`);
    }
  });

  test('returns empty string unchanged for empty oid', () => {
    expect(getCppIdentifier('')).toBe('');
  });

  test('returns identifier with underscores unchanged when not a keyword', () => {
    expect(getCppIdentifier('my_param_name')).toBe('my_param_name');
  });
});

/**
 * Builds a Device and populates device.constraints then device.params like CppGen.
 * @param {object} desc - device descriptor (params, constraints, slot, detail_level, etc.)
 * @param {string} deviceName - namespace/device name
 * @returns {{ device: Device, params: Record<string, Param> }}
 */
function buildDeviceWithParams(desc, deviceName = 'Test') {
  const deviceModel = { baseFilename: 'test.json', deviceName, desc };
  const device = new Device(deviceModel);
  if ('constraints' in device.desc) {
    for (const oid of Object.keys(device.desc.constraints)) {
      device.constraints[oid] = new Constraint(oid, device.desc.constraints[oid]);
    }
  }
  if ('params' in device.desc) {
    for (const oid of Object.keys(device.desc.params)) {
      device.params[oid] = new Param(
        oid,
        device.desc.params[oid],
        device.namespace,
        device
      );
    }
  }
  return { device, params: device.params };
}

describe('Param class', () => {

  const { device, params } = buildDeviceWithParams(MINIMAL_DESCRIPTOR_WITH_KEYWORDS, 'keywords');

  test('getFQOid returns correct path', () => {
    expect(params.product.getFQOid()).toBe('/product');
    expect(params.auto.getFQOid()).toBe('/auto');
    expect(params.product.getParam(['name']).getFQOid()).toBe('/product/name');
  });

  test('getParam returns correct param', () => {
    expect(params.product.getParam(['name']).oid).toBe('name');
    expect(params.product.getParam(['vendor']).oid).toBe('vendor');
    expect(params.product.getParam(['struct', 'nested']).oid).toBe('nested');
  });

  test('getParam returns undefined for missing path', () => {
    expect(params.product.getParam(['nonexistent'])).toBeUndefined();
  });

  // test('hasValue reflects descriptor value', () => {
  //   expect(params.product.hasValue()).toBe(true);
  //   expect(params.auto.hasValue()).toBe(false);
  // });

  test('getSubParams returns subparams for struct', () => {
    const subParams = params.product.getSubParams();
    expect(subParams.length).toBeGreaterThan(0);
    expect(subParams.map(p => p.oid)).toContain('name');
  });

  test('getSubParams returns subparams for templated param', () => {
    const subParams = params.item_list.getSubParams();
    expect(subParams.length).toBeGreaterThan(0);
    expect(subParams.map(p => p.oid)).toContain('sub');
  });

  test('getSubParams throws for non-struct type', () => {
    expect(() => params.auto.getSubParams()).toThrow(/does not have subparams/);
  });

  test('getAlternativeTypes throws for struct (not variant)', () => {
    expect(() => params.product.getAlternativeTypes()).toThrow(/does not have alternatives/);
  });

  test('elementType throws for non-array param', () => {
    expect(() => params.product.elementType()).toThrow(/does not have an element type/);
  });

  test('initializeValue without value returns uninitialized declaration', () => {
    const init = params.auto.initializeValue();
    expect(init).toMatch(/std::string\s+auto_/);
    expect(init).toContain(';');
  });

  test('initializeValue with value returns initializer', () => {
    const init = params.product.initializeValue();
    expect(init).toMatch(/Product\s+product/);
    expect(init).toContain('.name');
  });

  test('initializeValue with non-structs', () => {
    const NON_STRUCT_VALUES_DESCRIPTOR = {
      slot: 0,
      detail_level: 'FULL',
      access_scopes: ['st2138:mon', 'st2138:op'],
      default_scope: 'st2138:op',
      params: {
        variables: {
          type: 'STRUCT',
          value: {
            struct_value: {
              fields: {
                var1: { empty_value: '' },
                var2: { string_value: 'test' },
                var3: { int32_value: 10 },
                var4: { float32_value: 10.1 },
                var5: { string_array_values: { strings: ['a', 'b', 'c'] } },
                var6: { int32_array_values: { ints: [1, 2, 3] } },
                var7: { float32_array_values: { floats: [1.1, 2.2, 3.3] } },
              }
            }
          },
          params: {
            var1: { type: 'EMPTY' },
            var2: { type: 'STRING' },
            var3: { type: 'INT32' },
            var4: { type: 'FLOAT32' },
            var5: { type: 'STRING_ARRAY' },
            var6: { type: 'INT32_ARRAY' },
            var7: { type: 'FLOAT32_ARRAY' },
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(NON_STRUCT_VALUES_DESCRIPTOR, 'NonStruct');
    const init = params.variables.initializeValue();
    expect(init).toMatch(/Variables\s+variables/);
    expect(init).toContain('"test"');
    expect(init).toContain('.var3{10}');
    expect(init).toContain('.var4{10.1}');
    expect(init).toContain('"a"');
    expect(init).toContain('"b"');
    expect(init).toContain('"c"');
    expect(init).toContain('.var6{1, 2, 3}');
    expect(init).toContain('.var7{1.1, 2.2, 3.3}');
  });
  
  test('initializeValue with structs', () => {
    const STRUCT_VALUES_DESCRIPTOR = {
      slot: 0,
      detail_level: 'FULL',
      access_scopes: ['st2138:mon', 'st2138:op'],
      default_scope: 'st2138:op',
      params: {
        variables: {
          type: 'STRUCT',
          value: {
            struct_value: {
              fields: {
                var1: {
                  struct_value: {
                    fields: {
                      inner: { string_value: 'inner' }
                    }
                  }
                },
                var2: {
                  struct_array_values: {
                    struct_values: [
                      { fields: { x: { int32_value: 1 } } },
                      { fields: { x: { int32_value: 2 } } }
                    ]
                  }
                },
                var3: {
                  struct_variant_value: {
                    struct_variant_type: 'a',
                    value: { string_value: 'variant_a' }
                  }
                },
                var5: {
                  struct_variant_array_values: {
                    struct_variants: [
                      {
                        struct_variant_value: {
                          struct_variant_type: 'a',
                          value: { string_value: 'first' }
                        }
                      },
                      {
                        struct_variant_value: {
                          struct_variant_type: 'b',
                          value: { int32_value: 42 }
                        }
                      }
                    ]
                  }
                }
              }
            }
          },
          params: {
            var1: {
              type: 'STRUCT',
              params: {
                inner: { type: 'STRING' }
              }
            },
            var2: {
              type: 'STRUCT_ARRAY',
              params: {
                x: { type: 'INT32' }
              }
            },
            var3: {
              type: 'STRUCT_VARIANT',
              params: {
                a: { type: 'STRING' },
                b: { type: 'INT32' }
              }
            },
            var4: { type: 'STRUCT_VARIANTS' },
            var5: {
              type: 'STRUCT_VARIANT_ARRAY',
              params: {
                a: { type: 'STRING' },
                b: { type: 'INT32' }
              }
            }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(STRUCT_VALUES_DESCRIPTOR, 'Struct');
    const init = params.variables.initializeValue();
    expect(init).toMatch(/Variables\s+variables/);
    expect(init).toContain('.inner{"inner"}');
    expect(init).toContain('.x{1}');
    expect(init).toContain('.x{2}');
    expect(init).toContain('"variant_a"');
    expect(init).toContain('"first"');
    expect(init).toContain('42');
  });

  test('initializeParamWithValue format for param with value', () => {
    const s = params.product.initializeParamWithValue();
    expect(s).toContain('ParamWithValue<');
    expect(s).toContain('product');
    expect(s).toContain('_productParam');
  });

  test('initializeParamWithValue format for param without value', () => {
    const s = params.auto.initializeParamWithValue();
    expect(s).toContain('EmptyValue');
    expect(s).toContain('emptyValue');
  });

  test('descriptor getArgs with parent includes parent reference', () => {
    const nameParam = params.product.getParam(['name']);
    const args = nameParam.descriptor.getArgs('_product');
    expect(args).toContain('&_productDescriptor');
  });

  test('descriptor getArgs without parent uses nullptr', () => {
    const args = params.product.descriptor.getArgs('');
    expect(args).toContain('nullptr');
  });

  test('getFieldInfoTypes returns FieldInfo tuple types', () => {
    const types = params.product.getFieldInfoTypes();
    expect(types).toContain('FieldInfo<');
    expect(types).toContain('Product>');
  });

  test('getFieldInfoInit returns oid and member pointers', () => {
    const init = params.product.getFieldInfoInit();
    expect(init).toContain('"name"');
    expect(init).toContain('&Product::name');
  });

  test('objectType throws for unknown param type', () => {
    const device = {
      getParam: () => undefined,
      getConstraint: () => {
        throw new Error('Invalid constraint');
      }
    };
    const param = new Param('x', { type: 'UNKNOWN_TYPE' }, 'ns', device);
    expect(() => param.objectType()).toThrow(/Unknown type/);
  });

  test('valueInitializer string_value produces quoted string', () => {
    const result = params.auto.valueInitializer(
      { string_value: 'hello' },
      'STRING',
      params.auto
    );
    expect(result).toBe('{"hello"}');
  });

  test('valueInitializer throws when value type does not match param type', () => {
    expect(() =>
      params.auto.valueInitializer({ int32_value: 42 }, 'STRING', params.auto)
    ).toThrow(/Value type .* does not match param type/);
  });

  test('valueInitializer int32_value produces numeric initializer', () => {
    const device = {
      getParam: () => undefined,
      getConstraint: () => {
        throw new Error('Invalid constraint');
      }
    };
    const param = new Param('x', { type: 'INT32' }, 'ns', device);
    expect(param.valueInitializer({ int32_value: 42 }, 'INT32', param)).toBe('{42}');
  });

  test('valueInitializer empty_value produces empty initializer', () => {
    const device = {
      getParam: () => undefined,
      getConstraint: () => {
        throw new Error('Invalid constraint');
      }
    };
    const param = new Param('x', { type: 'EMPTY' }, 'ns', device);
    expect(param.valueInitializer({ empty_value: {} }, 'EMPTY', param)).toBe('{}');
  });

  test('valueInitializer struct_value produces member initializers', () => {
    const result = params.product.valueInitializer(
      params.product.value,
      'STRUCT',
      params.product
    );
    expect(result).toMatch(/\.name\s*\{/);
    expect(result).toMatch(/\.vendor\s*\{/);
    expect(result).toContain('"Keyword Test"');
  });

  test('valueInitializer struct_value throws when fields is undefined', () => {
    expect(() =>
      params.product.valueInitializer(
        { struct_value: {} },
        'STRUCT',
        params.product
      )
    ).toThrow('Struct value must have fields map');
  });

  test('param with constraint ref_oid uses shared constraint', () => {
    expect(params.level.constraint).toBeDefined();
    expect(params.level.constraint.oid).toBe('shared_range');
  });

  test('param with inline constraint gets new Constraint', () => {
    expect(params.volume.constraint).toBeDefined();
    expect(params.volume.constraint.parentParam).toBe(params.volume);
  });

  test('param with template_oid is templated and inherits from template', () => {
    expect(params.item_list.isTemplated()).toBe(true);
    expect(params.item_list.template_param).toBe(params.item);
    expect(params.item_list.minimal_set).toBe(true);
  });

  // test('templated STRING_ARRAY param objectType is vector of string', () => {
  //   expect(params.item_list.objectType()).toBe('std::vector<std::string>');
  // });
});

describe('objectType, objectNamespaceType, elementType, elementNamespaceType', () => {
  test('objectNamespaceType for simple type returns typeArg (no namespace)', () => {
    const { device, params } = buildDeviceWithParams(MINIMAL_DESCRIPTOR_WITH_KEYWORDS, 'keywords');
    expect(params.auto.objectNamespaceType()).toBe('std::string');
  });

  test('objectNamespaceType for struct returns namespace::Type', () => {
    const { params } = buildDeviceWithParams(MINIMAL_DESCRIPTOR_WITH_KEYWORDS, 'keywords');
    expect(params.product.objectNamespaceType()).toBe('keywords::Product');
  });

  test('objectNamespaceType for non-templated STRUCT_ARRAY returns namespace::Type', () => {
    const descriptorWithStructArray = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        row: {
          type: 'STRUCT_ARRAY',
          name: { display_strings: { en: 'Row' } },
          params: { x: { type: 'INT32', name: { display_strings: { en: 'X' } } } }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithStructArray, 'Test');
    expect(params.row.objectNamespaceType()).toBe('Test::Row');
  });

  test('elementType for non-templated STRUCT_ARRAY returns objectType_elem', () => {
    const descriptorWithStructArray = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        row: {
          type: 'STRUCT_ARRAY',
          name: { display_strings: { en: 'Row' } },
          params: { x: { type: 'INT32', name: { display_strings: { en: 'X' } } } }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithStructArray, 'Test');
    expect(params.row.elementType()).toBe('Row_elem');
  });

  test('elementNamespaceType for non-templated STRUCT_ARRAY returns namespace::elementType', () => {
    const descriptorWithStructArray = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        row: {
          type: 'STRUCT_ARRAY',
          name: { display_strings: { en: 'Row' } },
          params: { x: { type: 'INT32', name: { display_strings: { en: 'X' } } } }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithStructArray, 'Test');
    expect(params.row.elementNamespaceType()).toBe('Test::Row_elem');
  });

  test('elementNamespaceType throws for non-array param', () => {
    const { params } = buildDeviceWithParams(MINIMAL_DESCRIPTOR_WITH_KEYWORDS, 'keywords');
    expect(() => params.product.elementNamespaceType()).toThrow(/does not have an element type/);
  });

  // STRUCT_ARRAY templated on STRUCT (non-array template): objectType/objectNamespaceType return param name; elementType/elementNamespaceType return template
  const descriptorStructArrayOnStruct = {
    slot: 1,
    detail_level: 'FULL',
    access_scopes: ['test:op'],
    default_scope: 'test:op',
    params: {
      row: {
        type: 'STRUCT',
        name: { display_strings: { en: 'Row' } },
        params: { x: { type: 'INT32', name: { display_strings: { en: 'X' } } } }
      },
      row_list: {
        type: 'STRUCT_ARRAY',
        name: { display_strings: { en: 'Row List' } },
        template_oid: '/row'
      }
    }
  };

  test('objectType for STRUCT_ARRAY templated on STRUCT returns param type (non-array template)', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStruct, 'Test');
    expect(params.row_list.objectType()).toBe('Row_list');
  });

  test('objectNamespaceType for STRUCT_ARRAY templated on STRUCT returns namespace::param', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStruct, 'Test');
    expect(params.row_list.objectNamespaceType()).toBe('Test::Row_list');
  });

  test('elementType for STRUCT_ARRAY templated on STRUCT returns template objectNamespaceType', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStruct, 'Test');
    expect(params.row_list.elementType()).toBe('Test::Row');
  });

  test('elementNamespaceType for STRUCT_ARRAY templated on STRUCT returns template objectNamespaceType', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStruct, 'Test');
    expect(params.row_list.elementNamespaceType()).toBe('Test::Row');
  });

  // STRUCT_ARRAY templated on STRUCT_ARRAY: objectType/objectNamespaceType delegate to template; elementType/elementNamespaceType use template element
  const descriptorStructArrayOnStructArray = {
    slot: 1,
    detail_level: 'FULL',
    access_scopes: ['test:op'],
    default_scope: 'test:op',
    params: {
      row2: {
        type: 'STRUCT_ARRAY',
        name: { display_strings: { en: 'Row2' } },
        params: { y: { type: 'STRING', name: { display_strings: { en: 'Y' } } } }
      },
      row2_list: {
        type: 'STRUCT_ARRAY',
        name: { display_strings: { en: 'Row2 List' } },
        template_oid: '/row2'
      }
    }
  };

  test('objectType for STRUCT_ARRAY templated on STRUCT_ARRAY delegates to template', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStructArray, 'Test');
    expect(params.row2_list.objectType()).toBe('Row2');
  });

  test('objectNamespaceType for STRUCT_ARRAY templated on STRUCT_ARRAY delegates to template', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStructArray, 'Test');
    expect(params.row2_list.objectNamespaceType()).toBe('Test::Row2');
  });

  test('elementType for STRUCT_ARRAY templated on STRUCT_ARRAY returns template elementType', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStructArray, 'Test');
    expect(params.row2_list.elementType()).toBe('Row2_elem');
  });

  test('elementNamespaceType for STRUCT_ARRAY templated on STRUCT_ARRAY returns namespace::elementType', () => {
    const { params } = buildDeviceWithParams(descriptorStructArrayOnStructArray, 'Test');
    expect(params.row2_list.elementNamespaceType()).toBe('Test::Row2_elem');
  });
});

describe('valueInitializer struct and variant', () => {
  const descriptorWithStructArray = {
    slot: 1,
    detail_level: 'FULL',
    access_scopes: ['test:op'],
    default_scope: 'test:op',
    params: {
      row: {
        type: 'STRUCT_ARRAY',
        name: { display_strings: { en: 'Row' } },
        params: {
          x: { type: 'INT32', name: { display_strings: { en: 'X' } } }
        },
        value: {
          struct_array_values: {
            struct_values: [
              { fields: { x: { int32_value: 1 } } },
              { fields: { x: { int32_value: 2 } } }
            ]
          }
        }
      }
    }
  };

  test('valueInitializer struct_array_values produces list of struct initializers', () => {
    const { params } = buildDeviceWithParams(descriptorWithStructArray, 'Test');
    const result = params.row.valueInitializer(
      params.row.value,
      'STRUCT_ARRAY',
      params.row
    );
    expect(result).toContain('.x{1}');
    expect(result).toContain('.x{2}');
  });

  test('valueInitializer struct_variant_value produces variant initializer', () => {
    const descriptorWithVariant = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice: {
          type: 'STRUCT_VARIANT',
          name: { display_strings: { en: 'Choice' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    const value = {
      struct_variant_value: {
        struct_variant_type: 'a',
        value: { string_value: 'hello' }
      }
    };
    const result = params.choice.valueInitializer(value, 'STRUCT_VARIANT', params.choice);
    expect(result).toContain('std::string');
    expect(result).toContain('"hello"');
  });

  test('valueInitializer struct_variant_value throws when struct_variant_type missing', () => {
    const descriptorWithVariant = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice: {
          type: 'STRUCT_VARIANT',
          name: { display_strings: { en: 'Choice' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    expect(() =>
      params.choice.valueInitializer(
        { struct_variant_value: { value: { string_value: 'x' } } },
        'STRUCT_VARIANT',
        params.choice
      )
    ).toThrow('struct_variant_value must have struct_variant_type');
  });

  test('valueInitializer struct_variant_value throws when value field missing', () => {
    const descriptorWithVariant = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice: {
          type: 'STRUCT_VARIANT',
          name: { display_strings: { en: 'Choice' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    expect(() =>
      params.choice.valueInitializer(
        { struct_variant_value: { struct_variant_type: 'a' } },
        'STRUCT_VARIANT',
        params.choice
      )
    ).toThrow('struct_variant_value must have value field');
  });

  test('valueInitializer struct_variant_value throws for unknown alternative', () => {
    const descriptorWithVariant = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice: {
          type: 'STRUCT_VARIANT',
          name: { display_strings: { en: 'Choice' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    expect(() =>
      params.choice.valueInitializer(
        {
          struct_variant_value: {
            struct_variant_type: 'c',
            value: { string_value: 'x' }
          }
        },
        'STRUCT_VARIANT',
        params.choice
      )
    ).toThrow(/is not an alternative of choice/);
  });

  test('valueInitializer struct_variants wraps variant value', () => {
    const descriptorWithVariant = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice: {
          type: 'STRUCT_VARIANT',
          name: { display_strings: { en: 'Choice' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    const value = {
      struct_variants: {
        struct_variant_value: {
          struct_variant_type: 'b',
          value: { int32_value: 42 }
        }
      }
    };
    const result = params.choice.valueInitializer(value, 'STRUCT_VARIANTS', params.choice);
    expect(result).toContain('42');
  });

  test('valueInitializer struct_variant_array_values produces list of variant initializers', () => {
    const descriptorWithVariantArray = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: ['test:op'],
      default_scope: 'test:op',
      params: {
        choice_list: {
          type: 'STRUCT_VARIANT_ARRAY',
          name: { display_strings: { en: 'Choice List' } },
          params: {
            a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
            b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
          },
          value: {
            struct_variant_array_values: {
              struct_variants: [
                {
                  struct_variant_value: {
                    struct_variant_type: 'a',
                    value: { string_value: 'first' }
                  }
                },
                {
                  struct_variant_value: {
                    struct_variant_type: 'b',
                    value: { int32_value: 10 }
                  }
                }
              ]
            }
          }
        }
      }
    };
    const { params } = buildDeviceWithParams(descriptorWithVariantArray, 'Test');
    const result = params.choice_list.valueInitializer(
      params.choice_list.value,
      'STRUCT_VARIANT_ARRAY',
      params.choice_list
    );
    expect(result).toContain('"first"');
    expect(result).toContain('10');
  });
});

describe('Param class STRUCT_VARIANT', () => {
  const descriptorWithVariant = {
    slot: 1,
    detail_level: 'FULL',
    access_scopes: ['test:op'],
    default_scope: 'test:op',
    params: {
      choice: {
        type: 'STRUCT_VARIANT',
        name: { display_strings: { en: 'Choice' } },
        params: {
          a: { type: 'STRING', name: { display_strings: { en: 'A' } } },
          b: { type: 'INT32', name: { display_strings: { en: 'B' } } }
        }
      }
    }
  };

  test('getAlternativeTypes returns C++ types for variant alternatives', () => {
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    const types = params.choice.getAlternativeTypes();
    expect(types).toContain('std::string');
    expect(types).toContain('int32_t');
  });

  test('getAlternativeNames returns oids for variant', () => {
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    const names = params.choice.getAlternativeNames();
    expect(names).toContain('"a"');
    expect(names).toContain('"b"');
  });

  test('isVariantType true for STRUCT_VARIANT', () => {
    const { params } = buildDeviceWithParams(descriptorWithVariant, 'Test');
    expect(params.choice.isVariantType()).toBe(true);
    expect(params.choice.isStructType()).toBe(false);
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
