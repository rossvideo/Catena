/**
 * Generic constructor writer.
 * Subclasses should push argument initializers to the arguments array in their constructor.
 */
class CppCtor {
    /**
     * @param {object} desc catena object descriptor
     */
    constructor (desc) {
        this.arguments = [];
        this.desc = desc;
    }

    /**
     * @returns {string} comma delimited argument initializers
     */
    argsToString() {
        return this.arguments.map(arg => arg(this.desc)).join(', ');
    }
}

module.exports = CppCtor;