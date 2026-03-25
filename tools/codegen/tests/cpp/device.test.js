import { describe, expect, test } from '@jest/globals';
import Device from '../../cpp/device.js';
import Param from '../../cpp/param.js';
import Constraint from '../../cpp/constraint.js';

const MINIMAL_DESC = {
  slot: 7,
  detail_level: 'FULL',
  access_scopes: ['st2138:mon', 'st2138:op'],
  default_scope: 'st2138:op',
  params: {
    leaf: { type: 'STRING' },
    product: {
      type: 'STRUCT',
      params: {
        name: { type: 'STRING' }
      }
    }
  }
};

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
  return device;
}

describe('Device constructor', () => {
  test('stores deviceModel, desc, namespace, and empty maps', () => {
    const deviceModel = { baseFilename: 'x.json', deviceName: 'MyDevice', desc: MINIMAL_DESC };
    const device = new Device(deviceModel);
    expect(device.deviceModel).toBe(deviceModel);
    expect(device.desc).toBe(MINIMAL_DESC);
    expect(device.namespace).toBe('MyDevice');
    expect(device.params).toEqual({});
    expect(device.constraints).toEqual({});
    expect(device.commands).toEqual({});
  });
});

describe('Device argsToString', () => {
  test('joins slot, detail level, access scopes, default scope, multiset, subscriptions', () => {
    const desc = {
      slot: 42,
      detail_level: 'BASIC',
      access_scopes: ['a:b', 'c:d'],
      default_scope: 'c:d',
      multi_set_enabled: true,
      subscriptions: true
    };
    const device = new Device({ baseFilename: 'd.json', deviceName: 'Ns', desc });
    const s = device.argsToString();
    expect(s).toContain('42');
    expect(s).toContain('DetailLevel("BASIC")()');
    expect(s).toContain('{"a:b", "c:d"}');
    expect(s).toContain('"c:d"');
    expect(s).toMatch(/true.*true|true,\s*true/);
  });

  test('arguments are set to correct defaults when omitted', () => {
    const desc = { slot: 1, detail_level: 'FULL' };
    const device = new Device({ baseFilename: 'd.json', deviceName: 'Ns', desc });
    const s = device.argsToString();
    expect(device.argsToString()).toContain('{}');
    expect(device.argsToString()).toContain('""');
    expect(s).toContain('false');
    expect(s).not.toContain('true');
  });

  test('multi_set_enabled false when explicitly false', () => {
    const desc = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: [],
      multi_set_enabled: false,
      subscriptions: true
    };
    const device = new Device({ baseFilename: 'd.json', deviceName: 'Ns', desc });
    expect(device.argsToString()).toContain('false');
  });

  test('subscriptions false when explicitlyfalse', () => {
    const desc = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: [],
      multi_set_enabled: true,
      subscriptions: false
    };
    const device = new Device({ baseFilename: 'd.json', deviceName: 'Ns', desc });
    const parts = device.argsToString().split(', ');
    expect(parts[parts.length - 1]).toBe('false');
  });
});

describe('Device getParam', () => {
  test('returns top-level param for single segment fqoid', () => {
    const device = buildDeviceWithParams(MINIMAL_DESC, 'Test');
    expect(device.getParam('/leaf').oid).toBe('leaf');
  });

  test('returns nested param for multi-segment fqoid', () => {
    const device = buildDeviceWithParams(MINIMAL_DESC, 'Test');
    expect(device.getParam('/product/name').oid).toBe('name');
  });

  test('throws when top-level oid is missing', () => {
    const device = buildDeviceWithParams(MINIMAL_DESC, 'Test');
    expect(() => device.getParam('/missing')).toThrow(/Invalid template parameter/);
  });
});

describe('Device getConstraint', () => {
  const RANGE = {
    type: 'INT_RANGE',
    int32_range: { min_value: 0, max_value: 10 }
  };

  test('returns registered constraint', () => {
    const desc = {
      slot: 1,
      detail_level: 'FULL',
      access_scopes: [],
      constraints: { shared: RANGE }
    };
    const device = new Device({ baseFilename: 'd.json', deviceName: 'Ns', desc });
    device.constraints.shared = new Constraint('shared', RANGE);
    expect(device.getConstraint('shared').oid).toBe('shared');
  });

  test('throws when constraint oid is missing', () => {
    const device = new Device({
      baseFilename: 'd.json',
      deviceName: 'Ns',
      desc: { slot: 1, detail_level: 'FULL', access_scopes: [] }
    });
    expect(() => device.getConstraint('nope')).toThrow(/Invalid constraint nope/);
  });
});
