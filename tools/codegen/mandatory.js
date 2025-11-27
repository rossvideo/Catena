
/**
 * @brief Validates that all mandatory product parameters and scopes are present and have valid values
 * @param {object} deviceDesc The complete device model object (includes top-level properties and params)
 * @param {boolean} disableMandatoryEnforcement If true, skip validation and return early
 * @throws {Error} If mandatory parameters are missing or have invalid values (when enforcement enabled)
 */
export function validateRequiredParamsAndScopes(deviceDesc, disableMandatoryEnforcement = false) {
    if (disableMandatoryEnforcement) {
        return;
    }
    const REQUIRED_PARAMS = {
        "name": true,
        "vendor": true,
        "version": true,
        "catena_sdk": false,
        "catena_sdk_version": false,
        "serial_number": true
    };

    const REQUIRED_SCOPE = "st2138:mon";

    if (!deviceDesc || !deviceDesc.params || !deviceDesc.params.product) {
        throw new Error(`Missing mandatory product struct in params`);
    }

    if (deviceDesc.params.product.type !== 'STRUCT') {
        throw new Error(`Product parameter must be STRUCT type, not ${deviceDesc.params.product.type}`);
    }

    if (!deviceDesc.params.product.read_only) {
        throw new Error(`Product parameter must be read_only`);
    }

    const productParams = deviceDesc.params.product.params || {};
    const productValue = deviceDesc.params.product.value;
    const missing = [];
    const emptyValues = [];
    const invalidScopes = [];

    // Get scope sources
    const productScope = deviceDesc.params.product.access_scope;
    const defaultScope = deviceDesc.default_scope;

    // Helper function to derive effective scope for a parameter
    function getDerivedScope(param) {
        const dScope = param.access_scope || productScope || defaultScope || REQUIRED_SCOPE;
        if (dScope == REQUIRED_SCOPE) {
            return dScope;
        } else {
            return "INVALID";
        } 
    }

    Object.entries(REQUIRED_PARAMS).forEach(([key, checkValue]) => {
        const param = productParams[key];

        if (!param) {
            missing.push(key);
        } else {
            if (param.type !== 'STRING') {
                missing.push(`${key} (not STRING type)`);
                return;
            }

            // Derive the effective scope and check if it's correct
            const derivedScope = getDerivedScope(param);
            
            if (derivedScope == "INVALID") {
                invalidScopes.push(`${key} (derived scope is invalid, must be ('${REQUIRED_SCOPE}')`);
            }

            if (checkValue) {
                let stringValue;

                if (productValue && productValue.struct_value && productValue.struct_value.fields) {
                    const field = productValue.struct_value.fields[key];
                    if (field && field.string_value) {
                        stringValue = field.string_value;
                    }
                }

                if (!stringValue) {
                    emptyValues.push(`${key} (no value found)`);
                } else if (stringValue.trim() === '') {
                    emptyValues.push(`${key} (empty string value)`);
                }
            }
        }
    });

    const allIssues = [...missing.map(p => `${p} (missing field)`), ...emptyValues, ...invalidScopes];

    if (allIssues.length > 0) {
        throw new Error(`Invalid mandatory product parameters: ${allIssues.join(', ')}`);
    }
}