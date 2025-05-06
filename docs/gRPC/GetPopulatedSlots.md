::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# GetPopulatedSlots
The GetPopulatedSlots gRPC returns a list of a service's slots actively populated by device managers. This gRPC takes no input.

### OUT
A list of a service's slots actively populated by device managers.
```
message SlotList {
  	repeated uint32 slots = 1;
}
```

<div style="text-align: center">

[Next Page: ExecuteCommand](ExecuteCommand.html)

</div>