![Alt](../images/Catena%20Logo_PMS2191%20&%20White.png)

# GetValue
Gets the value of a parameter within a device model.

### IN
```
message GetValuePayload {
  uint32 slot = 1;          // Uniquely identifies the device at node scope.
  string oid = 2;           // Uniquely identifies the param at device scope
  uint32 element_index = 3; /* (UNUSED) When requesting single elements of
                             * array parameters, the element in the array to
                             * get */
}
```

### OUT
The value of the specified parameter.
```
/* Represents a dynamically typed value with straightforward mapping to
 * OGP/JSON.
 * Note that sfixed32 types are used to prefer marshalling performance
 * to wire size. */
message Value {
  oneof kind {

    UndefinedValue undefined_value = 1;

    Empty empty_value = 2; // Used for zero-argument commands

    sfixed32 int32_value = 3;

    float float32_value = 4;

    string string_value = 5;

    StructValue struct_value = 6;

    Int32List int32_array_values = 7;

    Float32List float32_array_values = 8;

    StringList string_array_values = 9;

    StructList struct_array_values = 10;

    DataPayload data_payload = 11; // Used for commands that accept or return data blobs

    StructVariantValue struct_variant_value = 12;

    StructVariantList struct_variant_array_values = 13;
  }
}
```

<div style="text-align: center">

[Next Page: MultiSetValue](gRPCDocs/MultiSetValue.html)

</div>