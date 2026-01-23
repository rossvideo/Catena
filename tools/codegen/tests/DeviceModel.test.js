import { DeviceModel } from '../DeviceModel.js';
import { beforeEach, expect, jest, test } from '@jest/globals';

describe("DeviceModel", () => {

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
    test("load simple", async () => {
        validateFnMock.mockResolvedValue({ valid: true, data: { slot: 555 } });
        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await dm.load();
        expect(dm.desc).toEqual({ slot: 555 });
        expect(dm.baseFilename).toBe("device.TestDevice.json");
        expect(dm.deviceName).toBe("TestDevice");
        expect(validateFnMock).toHaveBeenCalledWith("device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(1);
    });

    test("load relative path", async () => {
        validateFnMock.mockResolvedValue({ valid: true, data: { slot: 555 } });
        const dm = new DeviceModel("device.TestDevice.json", validatorMock);
        await dm.load();
        expect(dm.desc).toEqual({ slot: 555 });
        expect(dm.baseFilename).toBe("device.TestDevice.json");
        expect(dm.deviceName).toBe("TestDevice");
        expect(validateFnMock).toHaveBeenCalledWith("device", new URL(`file://${process.cwd()}/device.TestDevice.json`));
        expect(validateFnMock).toHaveBeenCalledTimes(1);
    });

    test("load url", async () => {
        validateFnMock.mockResolvedValue({ valid: true, data: { slot: 555 } });
        const dm = new DeviceModel("http://example.com/absolute/path/to/device.TestDevice.json", validatorMock);
        await dm.load();
        expect(dm.desc).toEqual({ slot: 555 });
        expect(dm.baseFilename).toBe("device.TestDevice.json");
        expect(dm.deviceName).toBe("TestDevice");
        expect(validateFnMock).toHaveBeenCalledWith("device", new URL("http://example.com/absolute/path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(1);
    });

    // Test for loading an invalid schema
    test("load invalid schema", async () => {
        const dm = new DeviceModel("/path/to/invalid.TestDevice.json", validatorMock);
        await expect(dm.load()).rejects.toThrow("File must be a device model, not invalid");
        expect(validateFnMock).not.toHaveBeenCalled();
    });

    // Test for loading when the validator returns invalid
    test("load invalid data", async () => {
        validateFnMock.mockResolvedValue({ valid: false });
        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await expect(dm.load()).rejects.toThrow("Resource file:///path/to/device.TestDevice.json is invalid");
        expect(validateFnMock).toHaveBeenCalledWith("device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(1);
    });

    // more complex load with imports
    test("load with imports", async () => {
        validateFnMock
            .mockResolvedValueOnce({
                valid: true, data: {
                    params: {
                        ParamA: {
                            import: { url: "params/param.ParamA.json" }
                        },
                        ParamB: {
                            type: "INT32",
                            value: 42
                        }
                    }
                }
            })
            .mockResolvedValueOnce({
                valid: true, data: {
                    type: "INT32",
                    value: 66
                }
            });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await dm.load(true);
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
        expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", new URL("file:///path/to/params/param.ParamA.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(2);
    });

    // test for loading an import that itself has an import
    test("load deeper imports", async () => {
        validateFnMock
            .mockResolvedValueOnce({
                valid: true, data: {
                    params: {
                        struct: {
                            params: {
                                ParamA: {
                                    import: { url: "params/param.ParamA.json" }
                                }
                            }
                        }
                    }
                }
            })
            .mockResolvedValueOnce({
                valid: true, data: {
                    params: {
                        SubParamA1: {
                            import: { url: "param.SubParamA1.json" }
                        }
                    }
                }
            })
            .mockResolvedValueOnce({
                valid: true, data: {
                    type: "STRING",
                    value: "Hello"
                }
            });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await dm.load(true);
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
        expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", new URL("file:///path/to/params/param.ParamA.json"));
        expect(validateFnMock).toHaveBeenNthCalledWith(3, "param", new URL("file:///path/to/params/param.SubParamA1.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(3);
    });

    // test for import with wrong schema
    test("load import wrong schema", async () => {
        validateFnMock.mockResolvedValueOnce({
            valid: true, data: {
                params: {
                    ParamA: {
                        import: { url: "commands/command.CommandA.json" }
                    }
                }
            }
        });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await expect(dm.load(true)).rejects.toThrow("Imported file commands/command.CommandA.json is not a param file");
        expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(1);
    });

    // test for import that fails validation
    test("load import invalid data", async () => {
        validateFnMock
            .mockResolvedValueOnce({
                valid: true, data: {
                    params: {
                        ParamA: {
                            import: { url: "params/param.ParamA.json" }
                        }
                    }
                }
            })
            .mockResolvedValueOnce({
                valid: false
            });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await expect(dm.load(true)).rejects.toThrow("Resource file:///path/to/params/param.ParamA.json is invalid");
        expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", new URL("file:///path/to/device.TestDevice.json"));
        expect(validateFnMock).toHaveBeenNthCalledWith(2, "param", new URL("file:///path/to/params/param.ParamA.json"));
        expect(validateFnMock).toHaveBeenCalledTimes(2);
    });

    // test for excessive import depth, this one just loops on itself
    test("load excessive import depth", async () => {
        validateFnMock.mockResolvedValue({
            valid: true, data: {
                params: {
                    ParamA: {
                        import: { url: "params/param.ParamA.json" }
                    }
                }
            }
        });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await expect(dm.load(true)).rejects.toThrow("Maximum import depth exceeded");
        // one more than max depth cause it doesn't count the first param level
        expect(validateFnMock).toHaveBeenCalledTimes(101);
        // first call is the device
        expect(validateFnMock).toHaveBeenNthCalledWith(1, "device", new URL("file:///path/to/device.TestDevice.json"));
        // the rest of them should be for the param imports
        for (const call of validateFnMock.mock.calls.slice(1)) {
            expect(call[0]).toBe("param");
        }
    });

    // test for excessive import depth, with an import loop
    test("load import loop", async () => {
        validateFnMock.mockImplementation((_, url) => {
            const path = url.pathname;
            let nextIndex = 0;
            if (path.endsWith("device.TestDevice.json")) {
                nextIndex = 0;
            } else if (path.endsWith("param.Param0.json")) {
                nextIndex = 1;
            } else if (path.endsWith("param.Param1.json")) {
                nextIndex = 2;
            } else if (path.endsWith("param.Param2.json")) {
                nextIndex = 0;
            } else {
                throw new Error(`Unexpected path ${path}`);
            }
            return new Promise((resolve) => {
                resolve({
                    valid: true, data: {
                        params: {
                            [`Param${nextIndex}`]: {
                                import: { url: `param.Param${nextIndex}.json` }
                            }
                        }
                    }
                });
            });
        });

        const dm = new DeviceModel("/path/to/device.TestDevice.json", validatorMock);
        await expect(dm.load(true)).rejects.toThrow("Maximum import depth exceeded");
        expect(validateFnMock).toHaveBeenCalledTimes(101);
    });
});
