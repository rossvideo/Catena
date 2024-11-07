::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png)
:::

# Value Objects

> Defined in `interface/param.proto`

![alt](images/Catena%20UML%20-%20Value.svg)

Ok. At first it looks confusing. This is because a param's `value` field is polymorphic depending on its `type`.

Protobuf needs to indicate the type of a value by wrapping it in an object named for the type. For example:

```json
"value": { "int32_value": 32 }
```
It's actually not too bad for simple types, or their arrays. It does get complicated for structured data, especially if the structure features deep nesting.

This is an example of a parameter with a multiply nested data structure.

```json
"location": {
      "type": "STRUCT",
      "name": {  "display_strings": {  "en": "Location"  } },
      "params": {
        "latitude": {
          "type": "FLOAT32",
          "name": {  "display_strings": {  "en": "Latitude" } },
          "value": { "float32_value": 37.7749 }
        },
        "longitude": {
          "type": "FLOAT32",
          "name": { "display_strings": {  "en": "Longitude" } },
          "value": { "float32_value": -122.4194 }
        },
        "altitude": {
          "type": "INT32",
          "name": {  "display_strings": { "en": "Altitude"  } },
          "value": { "int32_value": 100  }
        },
        "name": {
          "type": "STRING",
          "name": {  "display_strings": { "en": "Name" }
          },
          "value": {  "string_value": "San Francisco" }
        },
        "coords": {
          "type": "STRUCT",
          "params": {
            "x": { "type": "FLOAT32", "value": { "float32_value": 1 } },
            "y": { "type": "FLOAT32", "value": { "float32_value": 1 } },
            "z": { "type": "FLOAT32",  "value": { "float32_value": 1 } }
          }
        }
      },
      "value": {
        "struct_value": {
          "fields": {
            "latitude": {  "value": { "float32_value": 37.7749 } },
            "longitude": { "value": { "float32_value": -122.4194 } },
            "altitude": { "value": {  "int32_value": 100 } },
            "name": { "value": {  "string_value": "San Francisco" } },
            "coords": {
              "value": {
                "struct_value": {
                  "fields": {
                    "x": { "value": {  "float32_value": 10.0 } },
                    "y": { "value": {  "float32_value": 11.0 } },
                    "z": {  "value": { "float32_value": 12.0 } }
                  }
                }
              }
            }
          }
        }
      }
    }
```

Happily, the C++ SDK hides this complexity from the device developer. The parameter shown above can be accessed as a Plain Old Data Structure (PODS) like this:

```cpp
{% raw %}
// use reflection to build accessors to the struct members
// the macro creates the following PODS:
// struct Coords {
//   float x;
//   float y;
//   float z;    
// };
//
// There is an implementation limit of 15 struct members
//
struct Coords {
    REFLECTABLE_STRUCT(Coords, (float)x, (float)y, (float)z);
};

// note nested struct
struct Location {
    REFLECTABLE_STRUCT(Location, (Coords)coords, (float)latitude, (float)longitude, (int32_t)altitude,
                       (std::string)name);
};

// read & write the PODS using canonical C++
Location loc = {{91.f, 82.f, 73.f}, 10.0f, 20.0f, -30, "Old Trafford"}, loc2;
std::unique_ptr<ParamAccessor> locParam = dm.param("/location");
locParam->getValue(loc2);
std::cout << "Location: " << loc2.latitude << ", " << loc2.longitude << ", " << loc2.altitude << ", "
            << loc2.name << ", " << loc2.coords.x << ", " << loc2.coords.y << ", " << loc2.coords.z
            << '\n';
locParam->setValue(loc);
{% endraw %}
```

<div style="text-align: center">

[Next Page: Variants](Variants.md)

</div>