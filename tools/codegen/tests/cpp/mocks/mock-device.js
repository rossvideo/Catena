/* Copyright 2026 Ross Video Ltd */

import { jest } from '@jest/globals';

import Constraint from '../../../cpp/constraint.js';
import Param from '../../../cpp/param.js';

/**
 * Builds a device-shaped object for Param tests: same getParam/getConstraint
 * contract as {@link Device}, implemented with jest.fn so callers can assert
 * on template and constraint lookups without instantiating Device.
 *
 * @param {object} desc - device descriptor (params, constraints, slot, etc.)
 * @param {string} [deviceName='Test'] - namespace / device name
 * @returns {{ device: object, params: Record<string, Param> }}
 */
export function createMockDeviceWithParams(desc, deviceName = 'Test') {
  const deviceModel = { baseFilename: 'test.json', deviceName, desc };
  const device = {
    deviceModel,
    desc,
    namespace: deviceName,
    params: {},
    constraints: {},
    commands: {},
    getParam: jest.fn(),
    getConstraint: jest.fn()
  };

  device.getParam.mockImplementation((fqoid) => {
    const path = fqoid.split('/');
    path.shift();
    const front = path.shift();
    if (!(front in device.params)) {
      throw new Error(`Invalid template parameter ${fqoid}`);
    }
    if (path.length === 0) {
      return device.params[front];
    }
    return device.params[front].getParam(path);
  });

  device.getConstraint.mockImplementation((oid) => {
    if (device.constraints[oid] === undefined) {
      throw new Error(`Invalid constraint ${oid}`);
    }
    return device.constraints[oid];
  });

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
