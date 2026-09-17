import { afterEach, beforeEach, describe, expect, jest, test } from '@jest/globals';
import packageJson from '../package.json' with { type: "json" };
import {
    createLogger,
    ExitError,
    MANDATORY_OPTION,
    OUTPUT_OPTION,
    PROTOS_OPTION,
    QUIET_OPTION,
    sync,
    VERSION,
} from '../common.js';

// flush the microtask queue so the async IIFE inside sync() settles
const flush = () => new Promise(resolve => setImmediate(resolve));

describe("createLogger", () => {
    afterEach(() => {
        jest.restoreAllMocks();
    });

    test("returns console.log when not quiet", () => {
        expect(createLogger({ quiet: false })).toBe(console.log);
    });

    test("returns console.log when quiet is unset", () => {
        expect(createLogger({})).toBe(console.log);
    });

    test("returns a no-op that does not log when quiet", () => {
        const logSpy = jest.spyOn(console, 'log').mockImplementation(() => { });
        const logger = createLogger({ quiet: true });
        logger("hello");
        expect(logSpy).not.toHaveBeenCalled();
    });
});

describe("ExitError", () => {
    test("is an Error with default message and code", () => {
        const err = new ExitError();
        expect(err).toBeInstanceOf(Error);
        expect(err.name).toBe("ExitError");
        expect(err.message).toBe("");
        expect(err.code).toBe(1);
    });

    test("stores a custom message and code", () => {
        const err = new ExitError("boom", 42);
        expect(err.message).toBe("boom");
        expect(err.code).toBe(42);
    });
});

describe("sync", () => {
    let exitSpy;
    let errorSpy;

    beforeEach(() => {
        exitSpy = jest.spyOn(process, 'exit').mockImplementation(() => { });
        errorSpy = jest.spyOn(console, 'error').mockImplementation(() => { });
    });

    afterEach(() => {
        jest.restoreAllMocks();
    });

    test("runs the program without error handling on success", async () => {
        const program = { parseAsync: jest.fn().mockResolvedValue(undefined) };

        sync(program);
        await flush();

        expect(program.parseAsync).toHaveBeenCalledTimes(1);
        expect(errorSpy).not.toHaveBeenCalled();
        expect(exitSpy).not.toHaveBeenCalled();
    });

    test("reports an ExitError message and exits with its code", async () => {
        const program = { parseAsync: jest.fn().mockRejectedValue(new ExitError("bad", 7)) };

        sync(program);
        await flush();

        expect(errorSpy).toHaveBeenCalledWith("bad");
        expect(exitSpy).toHaveBeenCalledWith(7);
    });

    test("reports a generic error and exits with code 1", async () => {
        const err = new Error("kaboom");
        const program = { parseAsync: jest.fn().mockRejectedValue(err) };

        sync(program);
        await flush();

        expect(errorSpy).toHaveBeenCalledWith("Error:", err);
        expect(exitSpy).toHaveBeenCalledWith(1);
    });
});
