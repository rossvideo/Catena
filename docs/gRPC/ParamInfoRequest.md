::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# ParamInfoRequest
Gets a parameter's basic information from the device model connected to the specified slot

### IN
``` proto
/* Specifies the OID and potentially its subtree of
 * child parameters if the recursive flag is set. */
message ParamInfoRequestPayload {
  uint32 slot = 1;       // Uniquely identifies the device at node scope.
  string oid_prefix = 2; /* Uniquely identifies starting parameter relative to
                          * device.params. Specifying "" selects all top-level
                          * parameters. */
  bool recursive = 3;    /* Selects whether the param's child parameters should
                          * also be returned. */
}
```

### OUT
A stream of ParamInfoResponses

``` proto
message ParamInfoResponse {
  ParamInfo info = 1;
  uint32 array_length = 2; /* If this ParamInfoResponse is for an array 
                            * parameter, include the length of the array. */
}

message ParamInfo {
  string oid = 1;          // The parameter's OID.
  PolyglotText name = 2;   // The parameter's name to display in a client GUI.
  ParamType type = 3;      // The parameter's type.
  string template_oid = 4; /* The OID of the template from which this parameter
                            * was created. */
}
```

<div style="text-align: center">

[Next Page: SetValue](SetValue.html)

</div>