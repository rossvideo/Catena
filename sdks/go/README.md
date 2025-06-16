# Catena Go SDK

Let's start with some assumptions:

- The SDK shall provide a connection manager which will communicate with client applications and the service's business logic
- The service's business logic has no need to directly communicate with clients, it always uses the connection manager as an intermediary
- The SDK shall provide for the construction of in-memory representations of Device Models based on YAML/JSON representations
- The business logic "co-owns" the in-memory device model together with the connection manager. Before accessing the device model, each co-owner must acquire a lock to the DM
- The business logic and connection manager alert each other of changes they have made to the device model via channels that are designed for the purpose
- Two connection managers will be provided:
  - gRPC with binary serialization
  - REST with JSON serialization

## In-memory Device Model

If we replicate the approach for the C++ SDK we create 3 related objects for each parameter & command in the device model:

- A value object
- A param descriptor object
- A param with value object - one for each top-level parameter

The value objects are as small as possible. For example a parameter of 32-bit integer type has its value represented by a 32-bit integer in memory.

The param descriptor provides the connection manager with intelligent access to the value object:

- applies authorization
- applies constraints

The connection manager only ever accesses a piece of the device model state via a param descriptor object

The business logic, however, can simply `my_int32_param = 3;` but it then has the responsiblity of sending a message to the connection manager that `my_int32_param` has changed value which will allow the connection manager to relay the news to all connected clients.


`STRUCT` objects map directly to `struct` instances in-memory.

So the following param descriptor in a device model...

```json
{
    "location": {
        "type": "STRUCT",
        "params": {
            "longitude": {
                "type": "FLOAT32"
                "value": { "float32_value": 0.0 }
            },
            "latitude": {
                "type": "FLOAT32",
                "value": { "float32_value": 0.0 }
            }
        },
        "value": {
          "struct_value": {
            "longitude": 41.0,
            "latitude": 10.0
          }
        }
    }
}
```

Would define a user-defined type:

```go
type Location struct {
    longitude float32,
    latitude float32
}
```

And create an object in memory

```go
location := Location{longitude: 41.0, latitude 10.0}
```

Similarly `ARRAY` parameter types map to arrays in memory.

We need to discuss what `STRUCT_VARIANT` maps to, perhaps something close to this...

```go
type ValueType int

const (
    IntType ValueType = iota
    StringType
)

type Variant struct {
    Type  ValueType
    IntVal int
    StrVal string
}

func (v Variant) Print() {
    switch v.Type {
    case IntType:
        fmt.Println("int:", v.IntVal)
    case StringType:
        fmt.Println("string:", v.StrVal)
    }
}
```