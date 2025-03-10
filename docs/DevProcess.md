::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Device Development Process

The recommended process is hopefully simple:

1. Design your [device model](DeviceModel.html) as a set of JSON or YAML files in a file structure documented [here](Validation.html).
2. Create two files: `device_model.cpp` and `CMakeLists.txt`. `device_model.cpp` can be left empty for now.
3. Include the `.../common/include` directory in your `CMakeList.txt` file using `target_include_directories()`.
4. Place the three of these files together in a directory `device_model` and include it as a subdirectory in the parent directory's `CMakeLists.txt` file.
5. Build Catena following the steps documented [here](../sdks/cpp/docs/cpp_sdk_main_page.md). If the CMake file has been configured correctly, this should generate your device's code.
6. In your build folder, use `./path/to/your/device.json` to run your model. This should do nothing at the moment.
7. If working, add your buisness logic to `device_model.cpp`.

For example devices, see `sdks/cpp/common/examples`.

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

## gRPC Connections
Once you have generated a device following the steps above, you can make it a gRPC service by following the process below.
1. Include the `.../connections/gRPC/include` directory in your `CMakeList.txt` file using `target_include_directories()`.
2. Initialize a `CatenaServiceImpl` object with your generated device.
3. Register this service with a `grpc::ServerBuilder` and build the server.
4. Build Catena and run your server. 
5. You should now be able to make various gRPC requests to the server using Postman.
6. Coming soon ... use DashBoard beta to even get a GUI!

For example devices with gRPC connections, see `sdks/cpp/connections/gRPC/examples`.

```cpp
using grpc::Server;
Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// This initializes a gRPC Catena service with your device and builds a server.
void RunRPCServer(std::string addr) {
    grpc::ServerBuilder builder;
    grpc::EnableDefaultHealthCheckService(true);
    // Adding a listening port and completion queue.
    builder.AddListeningPort(addr, catena::getServerCredentials());
    std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
    // Getting flags from command line.
    std::string EOPath = absl::GetFlag(FLAGS_static_root);
    bool authz = absl::GetFlag(FLAGS_authz);
    // Initializing and registering a gRPC service with the generated device.
    CatenaServiceImpl service(cq.get(), dm, EOPath, authz);
    builder.RegisterService(&service);
    // Building the server and beginning loop.
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    globalServer = server.get();
    service.init();
    std::thread cq_thread([&]() { service.processEvents(); });
    // wait for the server to shutdown and tidy up
    server->Wait();
    cq->Shutdown();
    cq_thread.join();
}
```

<div style="text-align: center">

[Next Page: Security](Security.html)

</div>