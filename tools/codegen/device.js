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
    return `DetailLevel("${desc.detail_level}")`;
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
    if ("multiset" in desc) {
        ans = desc.multiset ? `true` : `false`;
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