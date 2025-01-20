::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Device Development Process

The recommended process is hopefully simple:

1. Design your device model as a set of JSON or YAML files in a file structure documented [here](Validation.html).
    1. [OPTIONAL] Install the recommended Red Hat extension redhat.vscode-yaml either through vscode or using the command `code --install-extension redhat.vscode-yaml`.
2. Use `full_service --device_model /path/to/your/device.json` to run your model.
3. Use Postman to get/set your parameters.
4. Coming soon ... use DashBoard beta to even get a GUI!
5. If working in C++, replace the placeholder business logic in `full_service.cpp` with your own.

All interactions with all attached clients are handled by the Catena Device Model.

In the C++ SDK the DeviceModel provides a signals/slots mechanism to tell your business logic when a parameter value has been changed by an attached client. It also provides `setValue` methods and alerts connected clients about the value changes if they're subscribed to the parameter that was altered. The architecture simplifies to:

![alt](images/Catena%20UML%20-%20Device%20Architecture.svg)

```cpp
// This is the business logic for the service
// This simple example just increments a number every second,
// and logs client updates
//
// construction of dm not shown
//
dm.valueSetByClient.connect([](const ParamAccessor &p, catena::ParamIndex idx, const std::string &peer) {
    catena::Value v;
    // note that the DeviceModel is locked, 
    // so use the non-locking version of getValue to avoid deadlock.
    p.getValue<false>(&v, idx);
    std::cout << "Client " << peer << " set " << p.oid() << " to: " << printJSON(v) << '\n';
    // a real service would do something with the value here
});
std::thread loop([&dm]() {
    // a real service would possibly send status updates, telemetry or audio meters here
    auto a_number = dm.param("/a_number");
    int i = 0;
    while (globalLoop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        a_number->setValue(i++);
    }
});
loop.detach();
```

<div style="text-align: center">

[Next Page: Security](Security.html)

</div>