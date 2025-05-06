::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# UpdateSubscriptions
Updates a client's subscriptions, affecting the output of the Connect and DeviceRequest RPCs on detail_level SUBSCRIPTIONS.

### IN
```
message UpdateSubscriptionsPayload {
  uint32 slot = 1;                  /* Uniquely identifies the device at node
                                     * scope. */
  repeated string added_oids = 2;   /* A list of object IDs to add to current
                                     * subscriptions (or a partial OID with a
                                     * trailing "*" to indicate all object IDs
                                     * with the same prefix) */
  repeated string removed_oids = 3; /* A list of object IDs to remove from
                                     * current subscriptions (or a partial OID
                                     * with a trailing "*" to indicate all
                                     * object IDs with the same prefix) */
}
```

### OUT
A stream of parameters added/removed from subscriptions.
```
/* A parameter */
message ComponentParam {
  string oid = 1;
  Param param = 2;
}
```

<div style="text-align: center">

[Next Page: GetParam](GetParam.html)

</div>