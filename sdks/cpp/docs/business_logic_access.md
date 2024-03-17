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

Whereas a service implementation / business logic will actually want something this:

```C++
struct Location {
    float latitude;
    float longitude;
};
```

Size `8` bytes.

How can the service correspond with the SDK's `DeviceModel` using simple C++ native types?

Let's look at the options...

## Options

1. do nothing - let the business logic eat protobuf!
1. provide serdes support that allows the business logic to deal in terms of C++ 'native' objects that get serialized/deserialized to/from the device model (which is one, potentially massive, protobuf object)
1. use native C++ types in the device model itself and push the serdes work into the RPC handlers.

Here are 3 sketches for what that would look like from the business logic's viewpoint, using the example of accessing the `location` param's `latitude` member:

```cpp
// option 1 - let them eat protobuf!
catena::Device dev; // initialization not shown

// reading
const float& latitude = dev.params().at("location").value().struct_value().fields().at("latitude").value().float_value();

// writing - code is illustrative of complexity and not necessarily correct
*(dev.mutable_params()->at("location").mutable_value()->mutable_struct_value()->mutable_fields()->at("latitude").mutable_value()->mutable_float_value()) = latitude;
```

```cpp
// option 2 provide serdes support, the device model is still protobuf, but
// accessors are provided to hide this fact

// DeviceModel contains a catena::Device and adds some functionality
catena::DeviceModel dm; // initialization not shown

// get a parameter accessor to the location parameter
std::unique_ptr<catena::ParamAccessor> locParam = dm.param("/location");
struct Location { REFLECTABLE_STRUCT((float) latitude, (float) longitude) };
Location loc;
locParam.getValue(loc);
std::cout << "latitude is: " << loc.latitude << std::endl;

// where REFLECTABLE_STRUCT is a fairly complex macro defined in Reflect.h that
// preserves compile time type information to enable the business logic to deal
// in terms of (almost) normal data types.

// get a parameter accessor to the location's latitude field
std::unique_ptr<catena::ParamAccessor> latParam = dm.param("/location/latitude");
latParam.setValue(10.0f);

// N.B. both getValue and setValue lock the device model's mutex to ensure 
// that the operations are threadsafe

// the get/set value methods are function templates that can correctly respond 
// to REFLECTABLE_STRUCTs being passed to them and use information stored by the 
// preprocessor to serdes between the device model's protobuf structures and the 
// native C++ ones used by the business logic
```

```cpp
// option 3 - the device model is made of native c++ types which are derived 
// from its JSON representation by an external code generator
#include "my_device_model.h"

My_device_model dm; // initialization not shown;
{
  My_device_model::Lock lock(dm); // assume My_device_model provides thread safety somehow
  const float& latitude = dm.params.location.latitude; // reading
  dm.params.location.latitude = 10.0f; // writing
} // lock is destroyed and the mutex freed here

// or perhaps...
{
    catena::sdk::IParam* locParam = dm.param("/location/latitude");
    
    // reading
    const float& latitude = locParam->getValue(catena::meta::TypeTag<float>);
    
    // writing
    locParam->setValue(10.0f);

    // where get/set Value methods are threadsafe and IParam is an abstract
    // interface class that is subclassed by class templates for each supported
    // type

}
```



## 
