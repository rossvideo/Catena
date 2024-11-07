::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

> Defined in `interface/param.proto`

# Variant Parameters

Variants are useful when you want to represent your part of your device as an array where some elements can be of a different type to the others.

If this isn't what you want to do [skip ahead](DevProcess.html).

An example of where this is useful is where you want to represent your devices inputs as an array. But have the ability to represent them like this:

```json
inputs: [
    {
        "ndi_input": "my_computer:5600"
    },
    {
        "2110_input": "224.0.100.3:5000",
        "packets_per_second": 1234
    }
]
```

Where inputs[0] has data for an NDI IP stream, inputs[2] for a 2110-20 stream. Note these representations are technically wrong for simplicity's sake.

<div style="text-align: center">

[Next Page: Device Development Process](DevProcess.html)

</div>