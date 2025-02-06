/*Copyright 2024 Ross Video Ltd
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
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


const CppCtor = require('./cppctor');

/**
 * @param {object} desc 
 * @returns value of the slot member as a string
 */
function slotArg(desc) {
    return `${desc.slot}`
}

/**
 * @param {object} desc 
 * @returns value of the detail_level member as a DetailLevel object
 */
function detailLevelArg(desc) {
    return `DetailLevel("${desc.detail_level}")()`;
}

/**
 * @param {object} desc 
 * @returns intializer for the access_scopes member
 */
function accessScopesArg(desc) {
    if ("access_scopes" in desc) {
        return `{${desc.access_scopes.map(scope => `"st-2138:${scope}"`).join(', ')}}`;
    } else {
        return `{}`;
    }
}


/**
 * @param {object} desc
 * @returns initializer for default_scope member
 */
function defaultScopeArg(desc) {
    let ans = `""`;
    if ("default_scope" in desc) {
        ans = `"st-2138:${desc.default_scope}"`;
    }
    return ans;
}

/**
 * @param {object} desc 
 * @returns initializer for multi_set_enabled member
 */
function multisetArg(desc) {
    let ans = `false`;
    if ("multi_set_enabled" in desc) {
        ans = desc.multi_set_enabled ? `true` : `false`;
    }
    return ans;
}

/**
 * @param {object} desc 
 * @returns initializer for subscriptions member
 */
function subscriptionsArg(desc) {
    let ans = `false`;
    if ("subscriptions" in desc) {
        ans = desc.subscriptions ? `true` : `false`;
    }
    return ans;
}



/**
 * @class Device
 * Create constructor arguments for catena::common::Device object
 * Maintains maps of the device parameters, constraints, and commands
 * @param {DeviceModel} deviceModel The device model object
 */
class Device extends CppCtor {
    constructor(deviceModel) {
        super(deviceModel.desc);
        this.arguments.push(slotArg);
        this.arguments.push(detailLevelArg);
        this.arguments.push(accessScopesArg);
        this.arguments.push(defaultScopeArg);
        this.arguments.push(multisetArg);
        this.arguments.push(subscriptionsArg);
        
        this.deviceModel = deviceModel;
        this.desc = deviceModel.desc;
        this.namespace = deviceModel.deviceName;
        this.params = {};
        this.constraints = {};
        this.commands = {};
    }

    /**
     * @breif get a parameter by its fully qualified oid
     * @param {string} fqoid the fully qualified oid of the parameter
     * @returns a Param object
     * @throws if the parameter does not exist
     */
    getParam(fqoid) {
        const path = fqoid.split('/');
        path.shift(); // remove leading empty string
        let front = path.shift();

        if (!front in this.params) {
            throw new Error(`Invalid template parameter ${fqoid}`);
        }

        if (path.length === 0) {
            return this.params[front];
        } else {
            return this.params[front].getParam(path);
        }
    }

    /**
     * @brief get a shared constraint
     * @param {string} oid the oid of the constraint
     * @returns a Constraint object
     * @throws if the constraint does not exist
     */
    getConstraint(oid) {
        if (this.constraints[oid] === undefined) {
            throw new Error(`Invalid constraint ${oid}`);
        }
        return this.constraints[oid];
    }

}

module.exports = Device;