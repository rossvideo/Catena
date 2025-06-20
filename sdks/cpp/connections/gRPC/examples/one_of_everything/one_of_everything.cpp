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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Example program containing one of everyhting.
 * @file one_of_everything.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Benjamin.whitten@rossvideo.com
 */

// device model
#include "device.one_of_everything.yaml.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>

// connections/gRPC
#include <ServiceImpl.h>
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

using grpc::Server;
using catena::gRPC::CatenaServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;
std::atomic<bool> fibLoop = false;
std::unique_ptr<std::thread> fibThread = nullptr;
std::atomic<bool> counterLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        fibLoop = false;
        counterLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // Use an oid to get a pointer to the command you want to define
    // In Catena, commands have IParam type
    std::unique_ptr<IParam> fibStart = dm.getCommand("/fib_start", err);
    assert(fibStart != nullptr);

    /* 
     * Define a lambda function to be executed when the command is called
     * The lambda function must take a catena::Value as an argument and return 
     * a catena::CommandResponse.
     * Starts a thread which updates the number_example parameter with the next
     * number of the Fibonacci sequence every second.
     */
    fibStart->defineCommand([](catena::Value value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([value]() -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            std::unique_ptr<IParam> intParam = dm.getParam("/number_example", err);
            
            // If the loop is already running, return an exception.
            if (fibThread) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details("Already running");
            // If the state parameter does not exist, return an exception.
            } else if (!intParam) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details(err.what());
            } else {
                fibLoop = true;
                // Detaching thread to update number_example with next number of
                // the Fibonacci sequence every second.
                fibThread = std::make_unique<std::thread>([intParam = std::move(intParam)]() {
                    auto& fibParam = *dynamic_cast<ParamWithValue<int32_t>*>(intParam.get());
                    uint32_t prev = 0;
                    uint32_t curr = 1;
                    while (fibLoop) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        uint32_t next = prev + curr;
                        prev = curr;
                        curr = next;
                        {
                        std::lock_guard lg(dm.mutex());
                        fibParam.get() = next;
                        dm.valueSetByServer.emit("/number_example", &fibParam);
                        }
                    }
                });

                std::cout << "Fibonacci sequence start" << std::endl;;
                response.mutable_no_response();
            }
            co_return response;
        }());
    });

    // This stops the looping thread in the fib_start command above.
    std::unique_ptr<IParam> fibStop = dm.getCommand("/fib_stop", err);
    assert(fibStop != nullptr);
    fibStop->defineCommand([](catena::Value value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([value]() -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            if (fibThread) {
                fibLoop = false;
                fibThread->join();
                fibThread = nullptr;
                std::cout << "Fibonacci sequence stop" << std::endl;
                response.mutable_no_response();
            } else {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details("Already stopped");
            }
            co_return response;
        }());
    });

    // This sets the value of number_example.
    std::unique_ptr<IParam> fibSet = dm.getCommand("/fib_set", err);
    assert(fibSet != nullptr);
    fibSet->defineCommand([](catena::Value value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([value]() -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            std::unique_ptr<IParam> intParam = dm.getParam("/number_example", err);
            if (intParam) {
                auto& fibParam = *dynamic_cast<ParamWithValue<int32_t>*>(intParam.get());
                fibParam.get() = value.int32_value();
                dm.valueSetByServer.emit("/number_example", &fibParam);
                response.mutable_no_response();
            } else {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details("/number_example not found");
            }
            co_return response;
        }());
    });

    // This fills float_array with 128 random floats rounded to 3 decimal places.
    std::unique_ptr<IParam> randomize = dm.getCommand("/randomize", err);
    assert(randomize != nullptr);
    randomize->defineCommand([](catena::Value value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([value]() -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            std::unique_ptr<IParam> floatArray = dm.getParam("/float_array", err);
            if (!floatArray) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details(err.what());
            } else {
                auto& floatArrayR = *dynamic_cast<ParamWithValue<std::vector<float>>*>(floatArray.get());
                std::vector<float>& randomArray = floatArrayR.get();
                randomArray.clear();
                // Generates random floats between 0 and 80 and rounds to 3 decimal places.
                for (int i = 0; i < floatArrayR.getDescriptor().max_length(); i++) {
                    float randomNum = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / 80);
                    randomNum = std::round(randomNum * 1000) / 1000;
                    randomArray.push_back(randomNum);
                }
                std::cout << "Randomized float array" << std::endl;
                response.mutable_no_response();
            }
            
            co_return response;
        }());
    });

    // This simulates a tape bot and returns a stream of responses.
    std::unique_ptr<IParam> tapeBot = dm.getCommand("/tape_bot", err);
    assert(tapeBot != nullptr);
    tapeBot->defineCommand([](catena::Value value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([value]() -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            // Locating
            std::cout << "Locating tape..." << std::endl;
            response.mutable_response()->set_string_value("Locating tape...");
            co_yield response;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Loading
            std::cout << "Tape found, loading..." << std::endl;
            response.Clear();
            response.mutable_response()->set_string_value("Tape found, loading...");
            co_yield response;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Seeking
            std::cout << "Tape loaded, seeking..." << std::endl;
            response.Clear();
            response.mutable_response()->set_string_value("Tape loaded, seeking...");
            co_yield response;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Reading
            std::cout << "File loaded, reading..." << std::endl;
            response.Clear();
            response.mutable_response()->set_string_value("File loaded, reading...");
            co_yield response;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Done
            std::cout << "File loaded." << std::endl;
            response.Clear();
            response.mutable_response()->set_string_value("File loaded.");
            co_return response;
        }());
    });
}

// Starts a loop on a detached thread that updates the counter parameter by 1
// every second. Resets to 0 when the counter reaches 200.
void startCounter() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    // The rest is the "sending end" of the status update example
    std::unique_ptr<IParam> param = dm.getParam("/counter", err);
    if (param == nullptr) {
        throw err;
    }
    // downcast the IParam to a ParamWithValue<int32_t>
    auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param.get());
    counter.get() = 0; // Initialize counter to 0
    while (counterLoop) {
        // update the counter once per second, and emit the event
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            std::lock_guard lg(dm.mutex());
            if (counter.get()++ >= 200) {
                counter.get() = 0; // Reset counter to 0 when it reaches 200
            }
            std::cout << counter.getOid() << " set to " << counter.get() << '\n';
            dm.valueSetByServer.emit("/counter", &counter);
        }
    }
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authz = absl::GetFlag(FLAGS_authz);
        CatenaServiceImpl service(cq.get(), dm, EOPath, authz);

        // Updating device's default max array length.
        dm.set_default_max_length(absl::GetFlag(FLAGS_default_max_array_size));

        builder.RegisterService(&service);


        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms) << '\n';

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        std::thread loop(startCounter);

        // wait for the server to shutdown and tidy up
        server->Wait();

        if (fibThread) { fibThread->join(); }
        loop.join();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

int main(int argc, char* argv[])
{
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  
    // commands should be defined before starting the RPC server 
    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    return 0;
}