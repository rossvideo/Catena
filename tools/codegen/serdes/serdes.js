/*Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file serdes.js
 * @brief functions to serialize and deserialize Catena device models
 * @author Andrew Brown <andrew.brown@rossvideo.com>
 * @date 2025-12-01
 * @copyright Copyright (c) 2025 Ross Video
 */

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
    const deviceDesc = deviceModel.desc;
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
    return [metadata, deviceDesc];
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
    const root = new protobuf.Root();
    await root.load(protoFiles.map(f => path.join(protoPath, f)), {
        keepCase: true, // so that protobufjs doesn't convert field names to camelCase
    });
    // throws if not found
    const device = root.lookupType("st2138.Device");
    return device;
}
