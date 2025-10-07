import { existsSync, readFileSync } from 'fs';
import { basename, dirname, extname } from "path";
import { parse } from 'yaml';

import Validator from 'smpte-validator';

/**
 * @class DeviceModel
 * @brief Holds the information parsed from a json device model file
 */
export class DeviceModel {
    /**
     * @brief Construct a new DeviceModel object
     * @param {string} filePath the path to the device model file
     * @param {Validator} validator the json validator object
     * @param {object} desc the parsed json object
     */
    constructor(filePath, validator, desc) {
        this.filePath = filePath;
        this.validator = validator;
        this.desc = desc;
        this.baseFilename = basename(filePath);
        const info = this.baseFilename.split(".");
        const schemaName = info[0];
        if (schemaName !== "device") {
            throw new Error(`File must be a device model, not ${schemaName}`);
        }
        this.deviceName = info[1];
    }

    /**
     * @brief open a param.*.json file and return the parsed json object
     * @param {string} importArg the path to the param.*.json file relative to the directory containing the device model file
     * @returns the parsed json object
     * @throws {Error} if the file cannot be opened or the data is invalid against the schema
     * @todo add support for other import types 
     */
    importParam(importArg) {
        if ("file" in importArg) {
            const importDir = dirname(this.filePath);
            const importPath = `${importDir}/${importArg.file}`;
            if (!existsSync(importPath)) {
                throw new Error(`Cannot open file at ${importArg.file}`);
            }
            // Determining if the file is yaml or json and parsing accordingly.
            const importData = (() => {
                const extension = extname(importPath);
                if (extension === ".yaml" || extension === ".yml") {
                    return parse(readFileSync(importPath, 'utf8'));
                } else { // Default
                    return JSON.parse(readFileSync(importPath));
                }
            })();
            return importData;

        } else {
            throw new Error(`Unsupported import type: ${importArg}`);
        }
    }
}
