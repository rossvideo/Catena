![Alt](../images/Catena%20Logo_PMS2191%20&%20White.png)

# DeviceRequest
The DeviceRequest gRPC returns a stream of components from the device manager at the specified slot. Which components are retrieved depends on the specified detail_level.
### IN
```
message DeviceRequestPayload {
  uint32 slot = 1;                     /* Uniquely identifies the device at
                                        * node scope. */
  string language = 2;                 /* Optionally specify the language for
                                        * the response */
  Device.DetailLevel detail_level = 3; /* Optionally specify the detail level
                                        * for the response */
  repeated string subscribed_oids = 4; /* Optionally specify a subscription
                                        * list for the device (see
                                        * UpdateSubscriptionsPayload) */
}
```
### OUT
A stream of DeviceComponents.
```
message DeviceComponent {
  /* A parameter or sub-parameter, or sub-sub-parameter, or ...
   *
   * These can be arbitrarily nested so a JSON pointer (RFC 6901) is used to
   * locate the component within the data model.
   * This is to allow large parameter trees to be broken into smaller
   * components.
   *
   * The json_pointer is relative to device["params"].
   * Let's see how this could be arranged for an audio processing device
   * which has a monitor output mix that has gain and eq controls.
   *
   * monitor - top level object id. The json_pointer is an object id in this
   * case. The whole multi-level parameter descriptor would form the body
   * of the message.
   *
   * monitor/eq, monitor/level - sub-params of the top-level monitor
   * parameter. The json_pointer is a cascade of object IDs.
   * Only the selected sub-param occupies the message body. Note that the eq
   * param is likely an array of 4 objects. Without an index in the
   * json_pointer, the whole array forms the body of the message.
   *
   * monitor/eq/1/f, monitor/eq/1/q, monitor/eq/1/gain sub-sub params of
   * eq 2 in the monitor output audio processing.
   * It's likely not practical to use messages this small to report device
   * models, but this illustrates how it could be done if desirable.
   */

  /* A parameter */
  message ComponentParam {
    string oid = 1;
    Param param = 2;
  }

   /* A menu.
    * Note that menus are grouped into menu groups. They are uniquely identified
    * by the json_pointer field which is relative to the Device's top-level
    * menu-groups object thus: menu-group-name/menu-name.
    * example: status/vendor_info */
  message ComponentMenu {
    string oid = 1;
    Menu menu = 2;
  }

   /* A constraint descriptor.
    * The oid is relative to the "constraints" child of the device's top-level
    * "types" object. */
  message ComponentConstraint {
    string oid = 1;
    Constraint constraint = 2;
  }

  /* A command descriptor.
   * Defines a command to be mapped to device.commands.oid */
  message ComponentCommand {
    string oid = 1;
    Param param = 2;
  }

  /* A language pack. */
  message ComponentLanguagePack {
    string language = 1; /* Language string is the language code that
                          * identifies the language e.g. en-uk */
    LanguagePack language_pack = 2;
  }

  /* Small device models can be sent as a whole.
   * Larger ones should be broken into a stream of smaller components. */
  oneof kind {
    Device device = 2;
    ComponentParam param = 3;
    ComponentConstraint shared_constraint = 4;
    ComponentMenu menu = 5;
    ComponentCommand command = 6;
    ComponentLanguagePack language_pack = 7;
  }
}
```

<div style="text-align: center">

[Next Page: GetPopulatedSlots](gRPCDocs/GetPopulatedSlots.html)

</div>