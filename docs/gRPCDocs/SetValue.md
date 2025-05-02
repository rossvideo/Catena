![Alt](../images/Catena%20Logo_PMS2191%20&%20White.png)

# SetValue
Sets the value of a parameter within a device model.

### IN
```
message SingleSetValuePayload {
  uint32 slot = 1;           // Uniquely identifies the device at node scope.
  SetValuePayload value = 2; // The param oid and value to apply
}

message SetValuePayload {
  string oid = 1;           // Uniquely identifies the param at device scope.
  uint32 element_index = 2; /* (UNUSED) When setting single elements of array
                             * parameters, the element in the array to update.
                             */
  Value value = 3;          // The value to apply
}
```

### OUT
This gRPC returns nothing.

<div style="text-align: center">

[Next Page: GetValue](gRPCDocs/GetValue.html)

</div>