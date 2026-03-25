import { describe, expect, test } from '@jest/globals';
import Constraint from '../../cpp/constraint.js';

describe('Constraint findType / constructor errors', () => {
  test('throws for unknown constraint type', () => {
    expect(() => new Constraint('c', { type: 'UNKNOWN' })).toThrow(/Unknown type UNKNOWN/);
  });

  test('ALARM_TABLE is in types map with empty implementation (no throw in findType)', () => {
    const c = new Constraint('a', { type: 'ALARM_TABLE' });
    expect(c.type).toBe('');
    expect(c.constraintType).toBe('');
  });
});

describe('Constraint shared vs param-owned', () => {
  test('shared constraint when parentParam is undefined', () => {
    const c = new Constraint('vol', {
      type: 'INT_RANGE',
      int32_range: { min_value: 0, max_value: 10 }
    });
    expect(c.isShared()).toBe(true);
    expect(c.variableName()).toBe('shared_vol');
    expect(c.parentParam).toBeUndefined();
  });

  test('param-owned constraint uses FQ oid in variableName', () => {
    const parentParam = {
      getFQOid: () => '/product/gain'
    };
    const c = new Constraint(
      'c',
      {
        type: 'FLOAT_RANGE',
        float32_range: { min_value: 0, max_value: 1, display_min: 0, display_max: 1 }
      },
      parentParam
    );
    expect(c.isShared()).toBe(false);
    expect(c.variableName()).toBe('_product_gainConstraint');
  });
});

describe('Constraint INT_RANGE', () => {
  const desc = {
    type: 'INT_RANGE',
    int32_range: {
      min_value: 0,
      max_value: 100,
      step: 5,
      display_min: 1,
      display_max: 99
    }
  };

  test('argsToString includes range fields', () => {
    const c = new Constraint('r', desc);
    const s = c.argsToString();
    expect(s).toContain('0');
    expect(s).toContain('100');
    expect(s).toContain('5');
    expect(s).toContain('1');
    expect(s).toContain('99');
  });

  test('step defaults to 0 when omitted', () => {
    const d = {
      type: 'INT_RANGE',
      int32_range: { min_value: 0, max_value: 10 }
    };
    const c = new Constraint('r', d);
    const parts = c.argsToString().split(', ');
    expect(parts).toContain('0');
  });

  test('display min/max default to min/max when omitted', () => {
    const d = {
      type: 'INT_RANGE',
      int32_range: { min_value: 2, max_value: 8 }
    };
    const c = new Constraint('r', d);
    const s = c.argsToString();
    expect(s).toMatch(/(^|,\s*)2(,|$)/);
    expect(s).toMatch(/8/);
  });

  test('getInitializer emits RangeConstraint and marks initialized', () => {
    const c = new Constraint('r', desc);
    expect(c.isInitialized()).toBe(false);
    const line = c.getInitializer();
    expect(c.isInitialized()).toBe(true);
    expect(line).toContain('RangeConstraint<int32_t>');
    expect(line).toContain('shared_r');
    expect(line).toContain('true, dm');
  });

  test('param-owned RangeConstraint initializer ends with shared false', () => {
    const parent = { getFQOid: () => '/gain' };
    const line = new Constraint(
      'c',
      { type: 'INT_RANGE', int32_range: { min_value: 0, max_value: 1 } },
      parent
    ).getInitializer();
    expect(line).toContain('_gainConstraint');
    expect(line).toMatch(/,\s*false\)\s*;/);
  });
});

describe('Constraint FLOAT_RANGE', () => {
  test('argsToString uses float32_range and step branch', () => {
    const desc = {
      type: 'FLOAT_RANGE',
      float32_range: {
        min_value: 0.0,
        max_value: 1.0,
        step: 0.1,
        display_min: 0.0,
        display_max: 1.0
      }
    };
    const c = new Constraint('f', desc);
    const s = c.argsToString();
    expect(s).toContain('0');
    expect(s).toContain('0.1');
  });

  test('getInitializer emits RangeConstraint<float>', () => {
    const desc = {
      type: 'FLOAT_RANGE',
      float32_range: { min_value: -1, max_value: 1, display_min: -1, display_max: 1 }
    };
    const line = new Constraint('f', desc).getInitializer();
    expect(line).toContain('RangeConstraint<float>');
  });

  test('display min/max for float default to min/max when display fields omitted', () => {
    const desc = {
      type: 'FLOAT_RANGE',
      float32_range: { min_value: 0.5, max_value: 2.5 }
    };
    const s = new Constraint('f', desc).argsToString();
    expect(s).toContain('0.5');
    expect(s).toContain('2.5');
  });
});

describe('Constraint INT_CHOICE', () => {
  test('getInitializer emits ChoiceConstraint with named choices', () => {
    const desc = {
      type: 'INT_CHOICE',
      int32_choice: {
        choices: [
          { value: 1, name: { display_strings: { en: 'One', fr: 'Un' } } },
          { value: 2, name: { display_strings: { en: 'Two' } } }
        ]
      }
    };
    const c = new Constraint('ch', desc);
    const line = c.getInitializer();
    expect(line).toContain('ChoiceConstraint<int32_t, st2138::Constraint::INT_CHOICE>');
    expect(line).toContain('{1,');
    expect(line).toContain('"en","One"');
    expect(line).toContain('"Two"');
    expect(line).toContain('false');
  });
});

describe('Constraint STRING_CHOICE', () => {
  test('simple string choices and strict flag', () => {
    const desc = {
      type: 'STRING_CHOICE',
      string_choice: {
        choices: ['low', 'high'],
        strict: true
      }
    };
    const line = new Constraint('sc', desc).getInitializer();
    expect(line).toContain('ChoiceConstraint<std::string, st2138::Constraint::STRING_CHOICE>');
    expect(line).toContain('"low"');
    expect(line).toContain('"high"');
    expect(line).toContain('true');
  });

  test('strict defaults to false when string_choice.strict omitted', () => {
    const desc = {
      type: 'STRING_CHOICE',
      string_choice: {
        choices: ['a']
      }
    };
    const line = new Constraint('sc', desc).getInitializer();
    expect(line).toContain('false');
  });

  test('strict false when explicitly false', () => {
    const desc = {
      type: 'STRING_CHOICE',
      string_choice: {
        choices: ['x'],
        strict: false
      }
    };
    const line = new Constraint('sc', desc).getInitializer();
    expect(line).toContain('false');
  });
});

describe('Constraint STRING_STRING_CHOICE', () => {
  test('choices with string value and display strings', () => {
    const desc = {
      type: 'STRING_STRING_CHOICE',
      string_string_choice: {
        choices: [
          { value: 'on', name: { display_strings: { en: 'On' } } }
        ],
        strict: true
      }
    };
    const line = new Constraint('ssc', desc).getInitializer();
    expect(line).toContain('ChoiceConstraint<std::string, st2138::Constraint::STRING_STRING_CHOICE>');
    expect(line).toContain('"on"');
    expect(line).toContain('"On"');
    expect(line).toContain('true');
  });

  test('strict from string_string_choice when false', () => {
    const desc = {
      type: 'STRING_STRING_CHOICE',
      string_string_choice: {
        choices: [{ value: 'v', name: { display_strings: { en: 'V' } } }],
        strict: false
      }
    };
    const line = new Constraint('ssc', desc).getInitializer();
    expect(line).toContain('false');
  });
});

describe('Constraint getInitializer idempotency', () => {
  test('second call still produces valid line (initialized stays true)', () => {
    const c = new Constraint('r', {
      type: 'INT_RANGE',
      int32_range: { min_value: 0, max_value: 1 }
    });
    const a = c.getInitializer();
    const b = c.getInitializer();
    expect(c.isInitialized()).toBe(true);
    expect(a).toBe(b);
  });
});
