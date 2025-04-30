/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// device model
#include "device.audio_meter.yaml.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>

// REST
#include "../../REST/include/ServiceImpl.h"

// gRPC
#include <SharedFlags.h>
#include "../../gRPC/include/ServiceImpl.h"
#include <ServiceCredentials.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>
#include <functional>

#include <iostream>

using namespace catena::common;
using grpc::Server;

Server *globalServer = nullptr;
catena::REST::CatenaServiceImpl *globalApi = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        globalLoop = false;
        // Shutting down REST
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
        }
        // Shutting down gRPC
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

// Starts the REST server.
void RunRESTServer(bool authorization, std::string EOPath) {
    try {
        // Creating and running the REST service.
        catena::REST::CatenaServiceImpl api(dm, EOPath, authorization, 443);
        globalApi = &api;
        std::cout << "API Version: " << api.version() << std::endl;
        std::cout << "REST on 0.0.0.0:443" << std::endl;        
        api.run();
    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

// Starts the gRPC server.
void RunGRPCServer(bool authorization, std::string EOPath) {
    try {
        std::string addr = absl::StrFormat("0.0.0.0:6256");

        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        CatenaServiceImpl service(cq.get(), dm, EOPath, authorization);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "gRPC on " << addr << std::endl;    

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    /*
     * Command to start the performance test.
     */
    std::unique_ptr<IParam> startCommand = dm.getCommand("/performance_test_start", err);
    assert(startCommand != nullptr);

    startCommand->defineCommand([](catena::Value value) {
        catena::CommandResponse response;
        globalLoop = true;
        for (int i = 0; i < 64; i ++) {
            std::thread loop([i = std::move(i)]() {
                std::string oid = "/audio_meter_list/"  + std::to_string(i) + "/audio_level";
                int counter = 0;
                while (globalLoop) {
                    catena::Value val;
                    val.set_float32_value(counter++);
                    // update the counter once per second, and emit the event
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    {
                        std::lock_guard lg(dm.mutex());
                        dm.setValue(oid, val);
                    }
                }
            });
            loop.detach();
        }
        std::cout << "Performance test started" << std::endl;
        response.mutable_no_response();
        return response;
    });
    /*
     * Command to end the performance test.
     */
    std::unique_ptr<IParam> endCommand = dm.getCommand("/performance_test_end", err);
    assert(endCommand != nullptr);

    endCommand->defineCommand([](catena::Value value) {
        catena::CommandResponse response;
        globalLoop = false;
        std::cout << "Performance test ended" << std::endl;
        response.mutable_no_response();
        return response;
    });
    /*
     * Command to simulate loading a tape.
     */
     std::unique_ptr<IParam> loadCommand = dm.getCommand("/load_tape", err);
     assert(loadCommand != nullptr);
 
     loadCommand->defineCommand([](catena::Value value) {
        catena::CommandResponse response;
        catena::Value state;
        std::vector<std::string> states = {"locating tape", "tape found, loading...", "tape loaded, seeking...", "file found, reading...", "file loaded"};

        // Simulating loading tape
        for (std::string& stateVal : states) {
            state.set_string_value(stateVal);
            dm.setValue("/state", state);
            std::cout << stateVal << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        response.mutable_no_response();
        return response;
     });
    /*
     * Command to switch the device's button on/off.
     */
    std::unique_ptr<IParam> pressCommand = dm.getCommand("/press_button", err);
    assert(pressCommand != nullptr);

    pressCommand->defineCommand([](catena::Value value) {
        catena::CommandResponse response;
        catena::Value buttonState;

        dm.getValue("/button", buttonState);
        std::string newState = buttonState.string_value() == "on" ? "off" : "on";
        buttonState.set_string_value(newState);
        dm.setValue("/button", buttonState);
        
        std::cout << "Button set to " << buttonState.string_value() << std::endl;
        *response.mutable_response() = buttonState;
        return response;
    });
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);

    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    // Getting flags.
    std::string EOPath = absl::GetFlag(FLAGS_static_root);
    bool authorization = absl::GetFlag(FLAGS_authz);
    
    defineCommands();

    // Running servers.
    std::thread catenaRestThread(RunRESTServer, authorization, EOPath);
    std::thread catenaGRPCThread(RunGRPCServer, authorization, EOPath);

    // Shutdown
    catenaRestThread.join();
    catenaGRPCThread.join();
    return 0;
}
