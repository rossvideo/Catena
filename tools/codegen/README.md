# Code Generator Documentation

## C++

The objective is to convert a Catena device model from JSON to a collection of files that contain native C++ objects and PODS based on the model's contents. These are then compiled as source for the service/device using the model.

Some parts of the model (such as the menu system, if present) are converted to binary protobuf files that are served by the service/device. (This is a forward looking statement).

The initial work is concentrating on stateful parameters.

### Conventions

Parameter names (their object ids) must be legal C++ object names. They must start with a lower-case letter in the range a-z.

The names of user-defined types i.e. `struct`s, and, later `variant`s are created from the object name by promoting the initial letter to uppercase.

All objects are declared within a namespace named for the service/device normally taken from the enclosing folder name.

### Example Inputs and Outputs

#### Let's start simple...

```json
"params": {
    "counter": {
        "type": "INT32"
    }
}
```

... has no impact on the generated header file and simply instantiates the object in the generated body file, and a Param object to register it with the device model.

```cpp
int32_t counter{};
catena::lite::Param<int32_t> counterParam(count,"/counter",dm);
```

#### Include a value in the JSON device model

```json
"params": {
    "counter": {
        "type": "INT32",
        "value": 42
    }
}
```

and note that it gets used to initialize the object.

```cpp
int32_t counter{42};
catena::lite::Param<int32_t> counterParam(count,"/counter",dm);
```

#### to do - arrays and other simple types, let's jump ahead to structs for now

#### User defined types

Catena enables parameters to be defined in terms of other parameters using its `template_oid` feature. Let's start with the case where a "type" is defined for its constraint alone.

```json
{
    "params": {
        "sexagesimal": {
            "type": "INT32",
            "value": 0,
            "constraint": {
                "type": "INT32_RANGE",
                "minvalue": 0,
                "maxvalue": 59,
                "step": 1
            }
        },
        "minutes": {
            "template_oid": "/sexagesimal",
            "value": 0
        },
        "seconds": {
            "template_oid": "/sexagesimal",
            "value": 0
        }
    }
}
```

Causes a type definition to be created in the generated header, as long as it hasn't already been added by a previous parameter `template_oid`-ing off of `/sexagesimal`.

```cpp
namespace my_device {
// could occur many lines higher, Sexagesimal is in the my_device namespace

using Sexagesimal = std::int32_t;


// could occur many lines lower
} // namespace device
```

And this in the body file:

```cpp
catena::lite::Constraint<catena::lite::int32_range> sexagesimalConstraint(0,59,1);
my_device::Sexagesimal minutes{0};
catena::lite::Param<int32_t> minutesParam(minutes,"/minutes",dm, sexagesimalConstraint);
my_device::Sexagesimal seconds{0};
catena::lite::Param<int32_t> minutesParam(seconds,"/seconds",dm, sexagesimalConstraint);
```

However, the reason to create a user-defined type that's a simple overlay of `std::int32_t` seems a bit strained. The main (only?) point in making `Sexagesimal` a type is to reuse the constraint defined in the descriptor. It has little to do with the objects.

Moreover, user-defined types based on primitives (and `std::vector`s of them) only become user-defined on use by a child parameter descriptor. As such, they're different from `STRUCT` and `STRUCT_VARIANT` which are user-defined on definition, not use. 

#### `template_oid` and data structures

consider this way of specifying a list of locations:

```json
{
    "constraints": {
        "sexagesimal": {
            "type": "INT_RANGE",
            "min_value": 0,
            "max_value": 59,
            "step": 1
        },
        "degrees_latitude": {
            "type": "INT_RANGE",
            "min_value": -90,
            "max_value": 90,
            "step": 1
        },
        "degrees_longitude": {
            "type": "INT_RANGE",
            "min_value": -180,
            "max_value": 180,
            "step": 1
        }
    },
    "params": {
        "latitude": {
            "type": "STRUCT",
            "params": {
                "degrees": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/degrees_latitude"
                    },
                    "value": 0
                },
                "minutes": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/sexagesimal"
                    },
                    "value": 0
                },
                "seconds": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/sexagesimal"
                    },
                    "value": 0
                }
            }
        },
        "longitude": {
            "type": "STRUCT",
            "params": {
                "degrees": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/degrees_longitude"
                    },
                    "value": 0
                },
                "minutes": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/sexagesimal"
                    },
                    "value": 0
                },
                "seconds": {
                    "type": "INT32",
                    "constraint": {
                        "ref_oid": "/sexagesimal"
                    },
                    "value": 0
                }
            }
        },
        "location": {
            "type": "STRUCT",
            "params": {
                "latitude": {
                    "template_oid": "/latitude"
                },
                "longitude": {
                    "template_oid": "/longitude"
                }
            }
        },
        "locations": {
            "type": "STRUCT_ARRAY",
            "template_oid": "/location",
            "value": {
                "struct_array_values": {
                    "struct_values": [
                        {
                            "fields": {
                                "latitude": {
                                    "struct_value": {
                                        "fields": {
                                            "degrees": {"int32_value": 1},
                                            "minutes": {"int32_value": 2},
                                            "seconds": {"int32_value": 3},
                                        }
                                    }
                                },
                                "longitude": {
                                    "struct_value": {
                                        "fields": {
                                            "degrees": {"int32_value": 4},
                                            "minutes": {"int32_value": 5},
                                            "seconds": {"int32_value": 6},
                                        }
                                    }
                                }
                            }
                        }
                    ]
                }
            }
        }
    }
}
```

The generated header should look something like (scope hierarchy omitted for readability):

```cpp
struct Latitude {
    int32_t degrees = 0,
    int32_t minutes = 0,
    int32_t seconds = 0
    static const StructInfo& getStructInfo();
};

struct Longitude {
    int32_t degrees = 0,
    int32_t minutes = 0,
    int32_t seconds = 0
    static const StructInfo& getStructInfo();
};

struct Location {
    Latitude latitude = {0,0,0};
    Longitude longitude = {0,0,0};
    static const StructInfo& getStructInfo();
};
```

And the body file something like this:

```cpp
// instantiate the constraints and add them to the device model, along
// with a key to locate them
Constraint<Int_Range> sexagesimalConstraint(dm, "/sexagesimal", 0,59,1);
Constraint<Int_Range> degreesLatitudeConstraint(dm, "/degrees_latitude", -90,90,1);
Constraint<Int_Range> degreesLongitudeConstraint(dm, "/degrees_longitude", -180,180,1);

Latitude latitude{};
Param<Latitude> latitudeParam(dm, latitude, "/latitude", degreesLatitudeConstraint);
const StructInfo& Latitude::getStructInfo() {
  static StructInfo t;
  if (t.name.length()) return t;
  t.name = "Latitude";
  FieldInfo fi;
    // register info for the degrees field
    fi.name = "degrees";
    fi.offset = offsetof(use_structs::Location::Latitude, degrees);
    fi.toProto = catena::lite::toProto<int32_t>;
    t.fields.push_back(fi);
    // register info for the minutes field
    fi.name = "minutes";
    fi.offset = offsetof(use_structs::Location::Latitude, minutes);
    fi.toProto = catena::lite::toProto<int32_t>;
    t.fields.push_back(fi);
    // register info for the seconds field
    fi.name = "minutes";
    fi.offset = offsetof(use_structs::Location::Latitude, seconds);
    fi.toProto = catena::lite::toProto<int32_t>;
    t.fields.push_back(fi);
  return t;
}

Longitude longitude{};
Location location{};
std::vector<Location> locations {{{1,2,3},{4,5,6}}};
```
