::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# GetParam
Gets a parameter within a device model.

### IN
```
message GetParamPayload {
  uint32 slot = 1; // Uniquely identifies the device at node scope.
  string oid = 2;  // Uniquely identifies the param at device scope.
}
```

### OUT
The specified parameter within the device mode.
```
/* A parameter */
message ComponentParam {
  string oid = 1;
  Param param = 2;
}
```

<div style="text-align: center">

[Next Page: Connect](Connect.html)

</div>