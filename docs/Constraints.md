::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Constraints

> Defined in `smpte/interface/proto/constraint.proto`

Constraints are applied to parameters to restrict the valid values that a parameter can take. They are optional and a parameter may only have at most one constraint. The supported constraints are:

- `ALARM_TABLE` - *not implemented yet*
- `INT_RANGE` - restricts a parameter to a range of integer values with an optional step size.
- `FLOAT_RANGE` - restricts a parameter to a range of float values with an optional step size.
- `INT_CHOICE` - restricts a parameter to a list of discrete integer values.
- `STRING_CHOICE` - restricts a parameter to a list of discrete string values.
- `STRING_STRING_CHOICE` - restricts a parameter to a list of discrete string values with associated display strings. This is useful when the valid values are not human readable.

## Range Constraints
Range constraints are applied to `INT32` and `FLOAT` parameter types. They specify a minimum and maximum value for the parameter, and an optional step size. If a step size is specified, the parameter's value must be a multiple of the step size away from the minimum value. For example, if the minimum value is -10, the maximum value is 10 and the step size is 4, valid values would be -10, -6, -2, 2, 6 and 10.

### Step Size Rounding
If a value is set that violates the step size constraint, it will be coerced to a valid value. For the case of `INT_RANGE`, the value will be rounded down to the nearest valid value. For the case of `FLOAT_RANGE`, the value will be rounded to the nearest valid value. If the number is exactly in between two valid values, it is undefined whether it will be rounded up or down.

## Choice Constraints

Choice constraints are applied to `INT32`, `STRING` and `STRING_STRING` parameter types. They specify a list of valid values for the parameter. For `INT_CHOICE` and `STRING_CHOICE`, the parameter's value must be one of the valid values specified in the constraint. For `STRING_STRING_CHOICE`, the parameter's value must be one of the valid values specified in the constraint, but each valid value also has an associated display string that can be used for GUI representation using a `PolyglotText` object.
