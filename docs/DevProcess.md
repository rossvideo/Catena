::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Device Development Process

The recommended process is hopefully simple:

1. Build the [Dev Container](DevContainer.html).
2. Design your [device model](DeviceModel.html) as a set of JSON or YAML files in a file structure documented [here](Validation.html).
3. Create two files: `device_model.cpp` and `CMakeLists.txt`. `device_model.cpp` can be left empty for now.
4. Include the `.../common/include` directory in your `CMakeList.txt` file using `target_include_directories()`.
5. Place the three of these files together in a directory `device_model` and include it as a subdirectory in the parent directory's `CMakeLists.txt` file.
6. Build Catena following the steps documented [here](doxygen/index.html). If the CMake file has been configured correctly, this should generate your device's code and create an executable named after your folder.
7. In your build folder, use `./path/to/your/device/folder/folderName` to run your model. This should do nothing at the moment.
8. If working, add your buisness logic to `device_model.cpp`.

For example devices, see `sdks/cpp/common/examples`.

All interactions with all attached clients are handled by the Catena Device Model.

In the C++ SDK the DeviceModel provides a signals/slots mechanism to tell your business logic when a parameter value has been changed by an attached client. It also provides `setValue` methods and alerts connected clients about the value changes if they're subscribed to the parameter that was altered. The architecture simplifies to:

![alt](images/Catena%20UML%20-%20Device%20Architecture.svg)

```cpp
// Global boolean so we can eventually exit the loop below.
bool globalLoop;

// This is the business logic for the service
// This simple example just increments a number every second,
// and logs client updates
//
// construction of dm not shown
//
dm.valueSetByClient_.connect([](const std::string& oid, const IParam* p) {
    auto value = p.get();
    DEBUG_LOG << "Client set " << p.oid() << " to: " << value << '\n';
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
1. Include the `catena_connections_grpc` library in your `CMakeList.txt` file using `target_link_libraries()`.
2. Initialize a `CatenaServiceImpl` object with your generated device.
3. Register this service with a `grpc::ServerBuilder` and build the server.
4. Build Catena and run your server. 
5. You should now be able to make various gRPC requests to the server using Postman.
6. Coming soon ... use DashBoard beta to even get a GUI!

For example devices with gRPC connections, see `sdks/cpp/connections/gRPC/examples`.

```cpp
using grpc::Server;
Server *globalServer = nullptr;

// This initializes a gRPC Catena service with your device and builds a server.
void RunRPCServer(std::string addr) {
    grpc::ServerBuilder builder;
    grpc::EnableDefaultHealthCheckService(true);
    // Adding a listening port and completion queue.
    builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
    std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
    // Getting command line flags
    ServiceConfig config = ServiceConfig()
        .set_EOPath(absl::GetFlag(FLAGS_static_root))
        .set_authz(absl::GetFlag(FLAGS_authz))
        .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
        .set_cq(cq.get())
        // Adding devices
        .add_dm(&dm);
    // Creating and registering catena service
    ServiceImpl service(config);
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

// Handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        if (globalServer != nullptr) {
            // This call shuts down the server. This should be called at the
            // end of all buisness logic.
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}
```

## REST Connections
Alternatively, you can make it a REST service by following the process below.
1. Include the `catena_connections_REST` library in your `CMakeList.txt` file using `target_link_libraries()`.
2. Initialize a `catena::REST::CatenaServiceImpl` object with your generated device.
3. Run you service with CatenaServiceImpl::Run()
5. You should now be able to make various REST API calls to the server using Postman or openAPI.
6. Coming soon ... use DashBoard beta to even get a GUI!

For example devices with REST connections, see `sdks/cpp/connections/REST/examples`.

```cpp
using <ServiceImpl.h>
catena::REST::CatenaServiceImpl *globalApi = nullptr;

// This initializes a REST Catena service with your device.
void RunRESTServer () {
    // Getting command line flags
    ServiceConfig config = ServiceConfig()
        .set_EOPath(absl::GetFlag(FLAGS_static_root))
        .set_authz(absl::GetFlag(FLAGS_authz))
        .set_port(absl::GetFlag(FLAGS_port))
        .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
        // Adding devices
        .add_dm(&dm);
    // Creating and running the REST service.
    ServiceImpl api(config);
    globalApi = &api;
    api.run();
}

// Handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        if (globalApi != nullptr) {
            // This call shuts down the server. This should be called at the
            // end of all buisness logic.
            globalApi->Shutdown();
            globalApi = nullptr;
        }
    });
    t.join();
}
```

<div style="text-align: center">

[Next Page: Security](Security.html)

</div>