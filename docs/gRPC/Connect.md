::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Connect
Connects a client to a service for updates on value changes.

### IN
```
/* Initial handshake between client and device. */
message ConnectPayload {
  string language = 1;                 // The language to recieve responses in.
  Device.DetailLevel detail_level = 2; // The detail level for the responses.
  string user_agent = 3;               /* Description of the client type and
                                        * version */
  bool force_connection = 4;           /* True to request access if connection
                                        * had been previously refused */
}
```

### OUT
A stream of PushUpdates from the server.
```
/* Messages that the device can push to the client. */
message PushUpdates {
  message PushExternalObject {
    string oid = 1;
    ExternalObjectPayload external_object = 2;
  }

  message PushValue {
    string oid = 1;
    int32 element_index = 2;
	Value value = 3;
  }

  uint32 slot = 1;
  
  oneof kind {
    PushValue value = 2; // e.g. audio meters
    DeviceComponent device_component = 3;
    Exception refused = 4;
    string invalidated_external_object_id = 5;
    TrapMessage trap_message = 6;
    bool invalidate_device_model = 7;
    PushExternalObject external_object = 8;
    RemoveComponents remove_device_components = 9;
    SlotList slots_added = 10;
    SlotList slots_removed = 11;
  }
}
```

<div style="text-align: center">

[Next Page: AddLanguage](../AddLanguage.html)

</div>