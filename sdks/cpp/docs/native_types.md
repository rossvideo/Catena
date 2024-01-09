# Native Types

Problem - Protobuf isn't a very accessible or (probably) performant way of representing the objects used by a service to control and report on its functionality of value.

Consider the JSON serialization of a simple location using Catena's abstraction:

```json
{
    "type": {
        "type": "STRUCT"
    },
    "name": {
        "display_strings": {    
            "en": "Location"
        }
    },
    "params": {
        "latitude": {
            "type": {
                "type": "FLOAT32"
            },
            "name": {
                "display_strings": {
                    "en": "Latitude"
                }
            },
            "value": {
                "float32_value": 37.7749
            }
        },
        "longitude": {
            "type": {
                "type": "FLOAT32"
            },
            "name": {
                "display_strings": {
                    "en": "Longitude"
                }
            },
            "value": {
                "float32_value": -122.4194
            }
        }
    },
    "value": {
        "struct_value": {
            "fields": {
                "latitude": {
                    "value":{
                        "float32_value": 37.7749
                    }
                },
                "longitude": {
                    "value":{
                        "float32_value": -122.4194
                    }
                }
            }
        }
    }
}
```

The size of the `catena::Value` object serialized as the `value` member of the param is `32` bytes on my ARM64 Mac.

Whereas a service implementation will actually want something this:

```C++
struct Location {
    float latitude;
    float longitude;
};
```

Size `8` bytes.

How can the service correspond with the SDK's `DeviceModel` using simple C++ native types?

Let's look at the options...

# Options

## 
