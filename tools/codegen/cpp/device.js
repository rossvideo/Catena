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
 * 
 * @param {object} desc 
 * @returns value of the detail_level member as a DetailLevel object
 */
function detailLevelArg(desc) {
    return `DetailLevel("${desc.detail_level}")()`;
}

/**
 * 
 * @param {object} desc 
 * @returns intializer for the access_scopes member
 */
function accessScopesArg(desc) {
    if ("access_scopes" in desc) {
        return `{${desc.access_scopes.map(scope => `Scope("${scope}")()`).join(', ')}}`;
    } else {
        return `{}`;
    }
}


/**
 * @param {object} desc
 * @returns initializer for default_scope member
 */
function defaultScopeArg(desc) {
    let ans = `{}`;
    if ("default_scope" in desc) {
        ans = `Scope("${desc.default_scope}")()`;
    }
    return ans;
}

/**
 * 
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
 * 
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
 * Create constructor arguments for catena::lite::Device object
 * @param {object} desc device descriptor
 */
class Device extends CppCtor {
    constructor(desc) {
        super(desc);
        this.arguments.push(slotArg);
        this.arguments.push(detailLevelArg);
        this.arguments.push(accessScopesArg);
        this.arguments.push(defaultScopeArg);
        this.arguments.push(multisetArg);
        this.arguments.push(subscriptionsArg);
    }

}

module.exports = Device;