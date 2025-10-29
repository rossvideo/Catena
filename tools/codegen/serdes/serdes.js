import { createHash } from 'crypto';
import fs from 'fs/promises';
import path from 'path';
import protobuf from 'protobufjs';
import { DeviceModel } from '../DeviceModel.js';

const MAGIC_STRING = "CTNA";

/**
 * Serialize the device model to a binary format
 * @param {DeviceModel} deviceModel the (resolved) device model
 * @param {object} options options from commander
 * @param {string} options.protos path to the protos directory
 * @param {string} options.output path to the output directory
 * @param {object} metadata additional metadata to include in the serialized output
 */
export async function serialize(deviceModel, options, metadata) {
    // convert the device model object from snake_case keys to camelCase keys
    // because protobufjs uses camelCase keys
    // e.g. device_model -> deviceModel
    //
    const deviceDesc = snakeToCamel(deviceModel.desc);
    const device = await getDevice(options.protos);
    const protoObj = device.fromObject(deviceDesc);
    const errMsg = device.verify(protoObj);
    /* istanbul ignore next don't know how to trigger this, but keeping for completeness */
    if (errMsg) {
        throw new Error(`Invalid device model: ${errMsg}`);
    }
    const deviceBin = device.encode(protoObj).finish();
    const metadataStr = JSON.stringify({
        ...metadata,
        payload: {
            size: deviceBin.length,
            sha256: createHash('sha256').update(deviceBin).digest('hex'),
        },
    });
    const metadataLen = Buffer.byteLength(metadataStr, "utf-8");
    const header = Buffer.alloc(8);
    header.write(MAGIC_STRING, 0, 4, "utf-8"); // magic
    header.writeUInt32LE(metadataLen, 4); // metadata length
    const outputBuffer = Buffer.concat([header, Buffer.from(metadataStr, "utf-8"), Buffer.from(deviceBin)]);
    await fs.mkdir(path.dirname(options.output), { recursive: true });
    const outputFile = path.join(options.output, `device.${deviceModel.deviceName}.bin`);
    await fs.writeFile(outputFile, outputBuffer, "binary");
}

/**
 * Deserialize the device model from a binary format
 * @param {object} options options from commander
 * @param {string} options.input path to the input file
 * @param {string} options.protos path to the protos directory
 * @returns {Promise<[object, object]>} the metadata object and the device model
 * @throws {Error} if the input file is invalid or deserialization fails
 */
export async function deserialize(options) {
    if (await fs.stat(options.input).catch(() => null) === null) {
        throw new Error(`Input file '${options.input}' does not exist`);
    }
    // returns a Buffer
    const fileBuffer = await fs.readFile(options.input);
    // verify magic number
    const magic = fileBuffer.subarray(0, 4).toString("utf-8");
    if (magic !== MAGIC_STRING) {
        throw new Error(`Not a serialized Catena device, got '${magic}'`);
    }
    const metadataLen = fileBuffer.readUInt32LE(4);
    const metadataStr = fileBuffer.subarray(8, 8 + metadataLen).toString("utf-8");
    const metadata = JSON.parse(metadataStr);
    const deviceBin = fileBuffer.subarray(8 + metadataLen);
    if (deviceBin.length !== metadata.payload.size) {
        throw new Error(`Protobuf size mismatch, expected ${metadata.payload.size}, got ${deviceBin.length}`);
    }
    if (createHash('sha256').update(deviceBin).digest('hex') !== metadata.payload.sha256) {
        throw new Error(`Protobuf SHA256 mismatch`);
    }
    const device = await getDevice(options.protos);
    const protoObj = device.decode(new Uint8Array(deviceBin));
    const deviceDesc = device.toObject(protoObj, {
        enums: String,
    });
    return [metadata, camelToSnake(deviceDesc)];
}

/**
 * Recursively convert all keys in an object from camelCase to snake_case. Makes a deep copy.
 * @param {*} obj the object to convert
 * @returns {*} a new object with snake_case keys
 */
export function camelToSnake(obj) {
    if (Array.isArray(obj)) {
        return obj.map(v => camelToSnake(v));
    } else if (obj !== null && obj.constructor === Object) {
        return Object.fromEntries(
            Object.entries(obj).map(([k, v]) => [
                k.replace(/([A-Z])/g, g => `_${g[0].toLowerCase()}`),
                camelToSnake(v)
            ])
        );
    }
    return obj;
}

/**
 * Load and return the protobuf root object
 * @param {string} protoPath path to the protos directory
 * @returns {Promise<protobuf.Message>} the protobuf root object
 * @throws {Error} if the protos path is invalid or loading fails
 */
export async function getDevice(protoPath) {
    if (await fs.stat(protoPath).catch(() => null) === null) {
        throw new Error(`Protos path '${protoPath}' does not exist`);
    }
    // load all the proto files in the protos directory
    const files = await fs.readdir(protoPath);
    const protoFiles = files.filter(f => f.endsWith('.proto'));
    const root = await protobuf.load(protoFiles.map(f => path.join(protoPath, f)));
    // throws if not found
    const device = root.lookupType("st2138.Device");
    return device;
}

/**
 * Recursively convert all keys in an object from snake_case to camelCase. Makes a deep copy.
 * @param {*} obj the object to convert
 * @returns {*} a new object with camelCase keys
 */
export function snakeToCamel(obj) {
    if (Array.isArray(obj)) {
        return obj.map(v => snakeToCamel(v));
    } else if (obj !== null && obj.constructor === Object) {
        return Object.fromEntries(
            Object.entries(obj).map(([k, v]) => [
                k.replace(/_([a-z])/g, g => g[1].toUpperCase()),
                snakeToCamel(v)
            ])
        );
    }
    return obj;
}
