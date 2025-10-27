import { DeviceModel } from '../DeviceModel.js';
import { beforeEach, expect, jest, test } from '@jest/globals';

const validateFnMock = jest.fn();
const validatorMock = {
    validate: validateFnMock
};

beforeEach(() => {
    jest.clearAllMocks();
});

// Constructor test
test("constructor", () => {
    const validator = {};
    const dm = new DeviceModel("/path/to/device.TestDevice.json", validator);
    expect(dm.url).toBe("/path/to/device.TestDevice.json");
    expect(dm.validator).toBe(validator);
    expect(dm.desc).toBe(null);
    expect(dm.baseFilename).toBe(null);
    expect(dm.deviceName).toBe(null);
});

// do a simple load with no imports
test("load simple", () => {
    validateFnMock.mockReturnValue({ valid: true, data: { slot: 555 } });
    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    dm.load();
    expect(dm.desc).toEqual({ slot: 555 });
    expect(dm.baseFilename).toBe("device.TestDevice.json");
    expect(dm.deviceName).toBe("TestDevice");
    expect(validateFnMock).toHaveBeenCalledWith("device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenCalledTimes(1);
});

// Test for loading an invalid schema
test("load invalid schema", () => {
    const dm = new DeviceModel("/path/to/invalid.TestDevice.json", validatorMock);
    expect(() => dm.load()).toThrow("File must be a device model, not invalid");
    expect(validateFnMock).not.toHaveBeenCalled();
});

// Test for loading when the validator returns invalid
test("load invalid data", () => {
    validateFnMock.mockReturnValue({ valid: false });
    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    expect(() => dm.load()).toThrow("Resource /path/to/device.TestDevice.json is invalid");
    expect(validateFnMock).toHaveBeenCalledWith("device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenCalledTimes(1);
});

// more complex load with imports
test("load with imports", () => {
    validateFnMock
        .mockReturnValueOnce({
            valid: true, data: {
                params: {
                    ParamA: {
                        import: { file: "params/param.ParamA.json" }
                    },
                    ParamB: {
                        type: "INT32",
                        value: 42
                    }
                }
            }
        })
        .mockReturnValueOnce({
            valid: true, data: {
                type: "INT32",
                value: 66
            }
        });

    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    dm.load(true);
    expect(dm.desc).toEqual({
        params: {
            ParamA: {
                type: "INT32",
                value: 66
            },
            ParamB: {
                type: "INT32",
                value: 42
            }
        }
    });
    expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", "/path/to/params/param.ParamA.json");
    expect(validateFnMock).toHaveBeenCalledTimes(2);
});

// test for loading an import that itself has an import
test("load deeper imports", () => {
    validateFnMock
        .mockReturnValueOnce({
            valid: true, data: {
                params: {
                    struct: {
                        params: {
                            ParamA: {
                                import: { file: "params/param.ParamA.json" }
                            }
                        }
                    }
                }
            }
        })
        .mockReturnValueOnce({
            valid: true, data: {
                params: {
                    SubParamA1: {
                        import: { file: "param.SubParamA1.json" }
                    }
                }
            }
        })
        .mockReturnValueOnce({
            valid: true, data: {
                type: "STRING",
                value: "Hello"
            }
        });

    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    dm.load(true);
    expect(dm.desc).toEqual({
        params: {
            struct: {
                params: {
                    ParamA: {
                        params: {
                            SubParamA1: {
                                type: "STRING",
                                value: "Hello"
                            }
                        }
                    }
                }
            }
        }
    });
    expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", "/path/to/params/param.ParamA.json");
    expect(validateFnMock).toHaveBeenNthCalledWith(3, "param", "/path/to/params/param.SubParamA1.json");
    expect(validateFnMock).toHaveBeenCalledTimes(3);
});

// test for import with wrong schema
test("load import wrong schema", () => {
    validateFnMock.mockReturnValueOnce({
        valid: true, data: {
            params: {
                ParamA: {
                    import: { file: "commands/command.CommandA.json" }
                }
            }
        }
    });

    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    expect(() => dm.load(true)).toThrow("Imported file commands/command.CommandA.json is not a param file");
    expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenCalledTimes(1);
});

// test for import that fails validation
test("load import invalid data", () => {
    validateFnMock
        .mockReturnValueOnce({
            valid: true, data: {
                params: {
                    ParamA: {
                        import: { file: "params/param.ParamA.json" }
                    }
                }
            }
        })
        .mockReturnValueOnce({
            valid: false
        });

    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    expect(() => dm.load(true)).toThrow("Resource /path/to/params/param.ParamA.json is invalid");
    expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", "/path/to/device.TestDevice.json");
    expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", "/path/to/params/param.ParamA.json");
    expect(validateFnMock).toHaveBeenCalledTimes(2);
});

// test for excessive import depth, this one just loops on itself
test("load excessive import depth", () => {
    validateFnMock.mockReturnValue({
        valid: true, data: {
            params: {
                ParamA: {
                    import: { file: "params/param.ParamA.json" }
                }
            }
        }
    });

    const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
    expect(() => dm.load(true)).toThrow("Maximum import depth exceeded");
    // one more than max depth cause it doesn't count the first param level
    expect(validateFnMock).toHaveBeenCalledTimes(101);
    // first call is the device
    expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", "/path/to/device.TestDevice.json");
    // the rest of them should be for the param imports
    for (const call of validateFnMock.mock.calls.slice(1)) {
        expect(call).toEqual(["param", expect.stringContaining("param.ParamA.json")]);
    }
});
