import { fileURLToPath } from 'url';
import fs from 'fs/promises';
import path from 'path';
import { deserialize, getDevice, serialize } from '../serdes/serdes.js';

describe("serdes", () => {

    const PROTOS_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../../smpte/interface/proto");
    const OUTPUT_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), "output");

    beforeAll(async () => {
        // clean out everything but the .gitignore file from the output directory
        const files = await fs.readdir(OUTPUT_DIR);
        for (const file of files) {
            if (file === ".gitignore") {
                continue;
            }
            await fs.rm(path.join(OUTPUT_DIR, file), { recursive: true, force: true });
        }
    });

    test("serdes", async () => {
        // easiest to test by serializing and then deserializing and comparing results
        const dm = {
            desc: {
                slot: 111,
                detail_level: "FULL",
                access_scopes: ["st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"],
                default_scope: "st2138:op",
                params: {
                    param_with_Caps: {
                        type: "INT32",
                        value: {
                            int32_value: 42,
                        },
                    },
                },
            },
            deviceName: "TestDevice",
        };
        await serialize(dm, {
            protos: PROTOS_DIR,
            output: OUTPUT_DIR,
        }, {
            version: 444,
        });
        const outputFile = path.join(OUTPUT_DIR, `device.TestDevice.bin`);
        expect(await fs.stat(outputFile)).toBeDefined();
        const [metadata, device] = await deserialize({
            input: outputFile,
            protos: PROTOS_DIR,
        });
        expect(metadata).toEqual({
            version: 444,
            payload: expect.objectContaining({
                sha256: expect.any(String),
                size: expect.any(Number),
            }),
        });
        expect(device).toEqual({
            slot: 111,
            detail_level: "FULL",
            access_scopes: ["st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"],
            default_scope: "st2138:op",
            params: {
                param_with_Caps: {
                    type: "INT32",
                    value: {
                        int32_value: 42,
                    },
                },
            },
        });
    });

    test("serialize creates output directory", async () => {
        const dm = {
            desc: {
                slot: 222,
                detail_level: "FULL",
                access_scopes: ["st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"],
                default_scope: "st2138:op",
                params: {},
            },
            deviceName: "TestDevice2",
        };
        const customOutputDir = path.join(OUTPUT_DIR, "custom_output_dir/subdir/nested");
        await serialize(dm, {
            protos: PROTOS_DIR,
            output: customOutputDir,
        }, {
            version: 555,
        });
        const outputFile = path.join(customOutputDir, `device.TestDevice2.bin`);
        expect(await fs.stat(outputFile)).toBeDefined();
    });

    test("deserialize invalid file", async () => {
        await expect(deserialize({
            input: path.join(OUTPUT_DIR, "nonexistent.bin"),
            protos: PROTOS_DIR,
        })).rejects.toThrow(/nonexistent.bin/);
    });

    test("deserialize invalid magic", async () => {
        // create a file with invalid magic
        const invalidFile = path.join(OUTPUT_DIR, "invalid_magic.bin");
        const buffer = Buffer.alloc(8);
        buffer.write("BAD!", 0, 4, "utf-8"); // bad magic
        buffer.writeUInt32LE(0, 4); // zero metadata length
        await fs.writeFile(invalidFile, buffer, "binary");
        await expect(deserialize({
            input: invalidFile,
            protos: PROTOS_DIR,
        })).rejects.toThrow("Not a serialized Catena device, got 'BAD!'");
    });

    test("deserialize invalid size", async () => {
        // create a file with valid magic but invalid size
        const invalidFile = path.join(OUTPUT_DIR, "invalid_size.bin");
        const header = Buffer.alloc(8);
        header.write("CTNA", 0, 4, "utf-8"); // magic
        const metadataObj = {
            payload: {
                size: 5,
                sha256: "dummyhash",
            },
        };
        const metadataStr = JSON.stringify(metadataObj);
        const metadata = Buffer.from(metadataStr, "utf-8");
        header.writeUInt32LE(metadata.length, 4); // metadata length
        const deviceBin = Buffer.from([0x00, 0x01, 0x02]); // 3 bytes, but we'll say it's 5
        const outputBuffer = Buffer.concat([header, metadata, deviceBin]);
        await fs.writeFile(invalidFile, outputBuffer, "binary");
        await expect(deserialize({
            input: invalidFile,
            protos: PROTOS_DIR,
        })).rejects.toThrow("Protobuf size mismatch");
    });

    test("deserialize invalid sha256", async () => {
        // create a file with valid magic but invalid sha256
        const invalidFile = path.join(OUTPUT_DIR, "invalid_sha256.bin");
        const header = Buffer.alloc(8);
        header.write("CTNA", 0, 4, "utf-8"); // magic
        const metadataObj = {
            payload: {
                size: 3,
                sha256: "invalidsha256hash",
            },
        };
        const metadataStr = JSON.stringify(metadataObj);
        const metadata = Buffer.from(metadataStr, "utf-8");
        header.writeUInt32LE(metadata.length, 4); // metadata length
        const deviceBin = Buffer.from([0x00, 0x01, 0x02]); // 3 bytes
        const outputBuffer = Buffer.concat([header, metadata, deviceBin]);
        await fs.writeFile(invalidFile, outputBuffer, "binary");
        await expect(deserialize({
            input: invalidFile,
            protos: PROTOS_DIR,
        })).rejects.toThrow("Protobuf SHA256 mismatch");
    });

    test("getDevice throws on missing protos", async () => {
        await expect(getDevice("/nonexistent/path")).rejects.toThrow("Protos path '/nonexistent/path' does not exist");
    });

    test("getDevice protos missing Device", async () => {
        // create an empty protos directory
        const emptyProtosDir = path.join(OUTPUT_DIR, "empty_protos");
        await fs.mkdir(emptyProtosDir, { recursive: true });
        await expect(getDevice(emptyProtosDir)).rejects.toThrow("st2138.Device");
    });
});
