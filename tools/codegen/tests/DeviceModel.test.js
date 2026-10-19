import { beforeEach, describe, expect, jest, test } from '@jest/globals';

// Mock every smpte-validator function used by DeviceModel.js. Must be
// registered before the module under test is (dynamically) imported.
jest.unstable_mockModule('smpte-validator', () => ({
    descriptorIdFromUrl: jest.fn(),
    formatDiagnostic: jest.fn(),
    resolve: jest.fn(),
}));

const { descriptorIdFromUrl, formatDiagnostic, resolve } = await import('smpte-validator');
const { resolveDeviceModel } = await import('../DeviceModel.js');
const { ExitError } = await import('../common.js');

describe("resolveDeviceModel", () => {
    const url = "file:///path/to/device.test.yaml";
    const options = { disableMandatoryEnforcement: false };
    let log;

    beforeEach(() => {
        jest.clearAllMocks();
        log = jest.fn();
    });

    test("resolves a valid device model", async () => {
        descriptorIdFromUrl.mockReturnValue({ kind: 'device', name: 'test' });
        resolve.mockResolvedValue({ valid: true, data: { some: 'tree' } });

        const dm = await resolveDeviceModel(url, log, options);

        expect(dm.baseFilename).toBe("device.test.yaml");
        expect(dm.deviceName).toBe("test");
        expect(dm.desc).toEqual({ some: 'tree' });
    });

    test("throws ExitError on malformed descriptor", async () => {
        descriptorIdFromUrl.mockImplementation(() => { throw new Error("malformed descriptor"); });
        await expect(resolveDeviceModel(url, log, options)).rejects.toThrow(ExitError);
        await expect(resolveDeviceModel(url, log, options))
            .rejects.toThrow("malformed descriptor");
        expect(resolve).not.toHaveBeenCalled();
    });

    test("logs a resolving message", async () => {
        descriptorIdFromUrl.mockReturnValue({ kind: 'device', name: 'test' });
        resolve.mockResolvedValue({ valid: true, data: {} });

        await resolveDeviceModel(url, log, options);

        expect(log).toHaveBeenCalledWith(`Resolving device model ${url}...`);
    });

    test("passes resolver options through to smpte-validator resolve", async () => {
        descriptorIdFromUrl.mockReturnValue({ kind: 'device', name: 'test' });
        resolve.mockResolvedValue({ valid: true, data: {} });

        await resolveDeviceModel(url, log, { disableMandatoryEnforcement: true });

        expect(resolve).toHaveBeenCalledWith(url, {
            disableMandatoryParams: true,
            sdkSuppliedProductParams: ['st2138_sdk', 'st2138_sdk_version'],
        });
    });

    test("throws ExitError when the descriptor is not a device", async () => {
        descriptorIdFromUrl.mockReturnValue({ kind: 'param', name: 'test' });

        await expect(resolveDeviceModel(url, log, options)).rejects.toThrow(ExitError);
        await expect(resolveDeviceModel(url, log, options))
            .rejects.toThrow("File must be a device model, not param");
        expect(resolve).not.toHaveBeenCalled();
    });

    test("throws ExitError with joined diagnostics when resolution is invalid", async () => {
        descriptorIdFromUrl.mockReturnValue({ kind: 'device', name: 'test' });
        resolve.mockResolvedValue({
            valid: false,
            diagnostics: [{ id: 1 }, { id: 2 }],
        });
        formatDiagnostic.mockImplementation(d => `diag-${d.id}`);

        await expect(resolveDeviceModel(url, log, options))
            .rejects.toThrow("diag-1\ndiag-2");
        expect(formatDiagnostic).toHaveBeenCalledTimes(2);
    });
});