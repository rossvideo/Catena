::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Param Descriptors

> Defined in `interface/param.proto`

![alt](images/Catena%20UML%20-%20Param.svg)

Param Descriptors describe these facets of a piece of your device's state:

- Its type. This is the only part of the descriptor that's mandatory. Supported types are shown in the UML diagram, above.
- Its value, this is polymorphic dependent on the value of the type field. See [here](Value.html) for more information.
- Constraints on valid values. These vary according to the type but include pick lists, ranges and something called an [AlarmTable](AlarmTable.html) which is a bit field of different alarms that your device may wish to raise.
- Child parameters, for `STRUCT`, `STRUCT_ARRAY`, `STRUCT_VARIANT` and `STRUCT_VARIANT_ARRAY` param types.
- Child commands.
- A widget hint to the client to use an appropriate GUI element to represent and adjust the parameter's value. These are currently client specific. It may be useful for us to define a [common subset](https://github.com/rossvideo/Catena/issues/92).
- client hints - these add things like scroll bars to some GUI widgets and are highly client specific. They can be ignored as far as the business logic of a device is concerned.
- A name as a [PolyglotText](PolyglotText.html) object. Clients use this object to label its representation in the GUI.
- A self-explanatory `read_only` flag
- A `stateless` flag to indicate that the parameter shouldn't be persisted. Examples where this would be used are: time of day and audio meters.
- and the [aforementioned](Template.html) `template_oid`

The SDKs provide convenient access to the parameters managed by a `DeviceModel` similar to this pseudo code:

```
// initialize the device model from file
DeviceModel dm(filename_of_root_device.json)

// the statements up to *** would be in your 
// device's control business logic

// get a handle on the audio parameters
gainParam = dm.getParam(/AudioSlot/gain)
meterParam = dm.getParam(/Audiometer)

// listen for valueSetByClient events
on valueSetByClient event (param, index, client) {
    if (param == gainParam) // This is one I can act on
        // use the handle to get the gain in dB
        dB = gainParam.getValue()
        // calculate the gain coefficient
        gain = power(10,db/20)
}

// *** the next 2 code blocks would be in your 
// program's main processing loop
every sample {
    audioOut = audioIn * gain;
}
every now & then {
    // compute the meter value
    meterLevel = calculateAudioMeter(audioOut)
    // write it to the data model which will relay
    // the change to connected clients
    meterParam.setValue (meterLevel)
}
```

<div style="text-align: center">

[Next Page: Value](Value.html)

</div>