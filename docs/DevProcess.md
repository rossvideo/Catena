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
dm.valueSetByClient.connect([](const std::string& oid, const IParam* p, const int32_t idx) {
    auto value = p.get();
    std::cout << "Client set " << p.oid() << " to: " << value << '\n';
    // a real service would do something with the value here
});
std::thread loop([&dm]() {
    // a real service would possibly send status updates, telemetry or audio meters here
    std::unique_ptr<IParam> param = dm.getParam("/counter", err);
    auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param.get());
    counter.get() = 0;
    while (globalLoop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter.get()++;
    }
});
loop.detach();
```

<div style="text-align: center">

[Next Page: Security](Security.html)

</div>