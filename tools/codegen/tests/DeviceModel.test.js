import { throwNoneDevice, toDeviceModel } from '../DeviceModel.js';
import { beforeEach, expect, jest, test } from '@jest/globals';

// make sure it fills in all the fields as expected
test("toDeviceModel", () => {
    const url = "file:///path/to/device.test.yaml";
    const desc = { slot: 555 };
    const dm = toDeviceModel(url, desc);
    expect(dm.baseFilename).toBe("device.test.yaml");
    expect(dm.deviceName).toBe("test");
    expect(dm.desc).toBe(desc);
})

describe("throwNoneDevice", () => {
    test("throws an error", () => {
        expect(() => throwNoneDevice("param.notdevice.yaml")).toThrow("ile must be a device model, not param");
    });

    test("doesn't throw an error for a valid device model filename", () => {
        expect(() => throwNoneDevice("device.test.yaml")).not.toThrow();
    });
});
