::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# ExecuteCommand
Executes a command on the device model connected to the specified slot.

### IN
```
message ExecuteCommandPayload {
  uint32 slot = 1;  // Slot id of the device
  string oid = 2;   /* The object id of the command to execute relative to
                     * device.commands */
  Value value = 3;  // The command's parameter value, if any
  bool respond = 4; // False if client wants to surpress response from server
  bool proceed = 5; // Used to short circuit sending payloads 
}
```

### OUT
Either a value, exception, boolean, or nothing.
```
/* When streamed, only the response field will be used after the first message. */
message CommandResponse {
  oneof kind {
    Empty no_response = 1;    /* Server/Device does not have a response for the
                               * command. */
    Value response = 2;
    Exception exception = 3;  /* This is for Server/Device error executing the
                               * command. */
    bool proceed = 4;         // Used to short circuit sending payloads 
  }
}

message Exception {
  string type = 1;
  PolyglotText error_message = 2;
  string details = 3;
}
```

<div style="text-align: center">

[Next Page: ExecuteCommand](ExternalObjectRequest.html)

</div>