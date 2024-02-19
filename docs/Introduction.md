
![Alt](images/Catena%20Logo_PMS2191%20&%20White.png)

# Introduction

The intended audience is developers who wish to incorporate Catena into a device or service.

For brevity, the word "device" will be used to mean either a device or a service going forward.

Catena is a secure, cloud scale control plane targeted at the live content production industry.

It's useful to use the following model when encountering Catena for the first time.


![Alt](images/Catena%20UML%20-%20Layers.svg)

- Catena is a self-describing service. The design motivation is to keep Catena clients "dumb" in the sense that they need no prior knowlege about a device in order to present a reasonable user interface with access to the device's resources.
- It defines the messages that are used to describe the service using the `protobuf` interface definition language, currently at `proto3`. `protobuf` is used to serialize messages across the wire. One of its many useful features is that the data can be represented as either a highly efficient binary representation, or as human readable (and writeable!) JSON.
- Connection management is outside of Catena's scope. So the diagram shows two reasonable options. The example programs in this repo use gRPC.
- For gRPC transport is via http/2 which enables messages with fast transaction times (e.g. get a parameter value) to overtake slower ones (e.g. file upload).

<div style="text-align: center">

[Next Page](DeviceModel.md)

</div>