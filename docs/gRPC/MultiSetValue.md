::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# MultiSetValue
Sets the value of multiple parameters within a device model.

### IN
``` proto
message MultiSetValuePayload {
  uint32 slot = 1;                     /* Uniquely identifies the device at
                                        * node scope. */
  repeated SetValuePayload values = 2; // The param oids and values to apply.
}

message SetValuePayload {
  string oid = 1;           // Uniquely identifies the param at device scope.
  Value value = 3;          // The value to apply
}
```

### OUT
This gRPC returns nothing.

<div style="text-align: center">

[Next Page: UpdateSubscriptions](UpdateSubscriptions.html)

</div>