// Copyright 2024 Ross Video Ltd
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @brief Example program to demonstrate using commands.
 * @file use_commands.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John Danen (john.danen@rossvideo.com)
 */

// device model
#include "device.video_player.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

// protobuf interface
#include <interface/service.grpc.pb.h>

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

using grpc::Server;

using namespace catena::common;

Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}



void statusUpdate(){   
    std::thread loop([]() {
        // this is the "receiving end" of the status update example
        dm.valueSetByClient.connect([](const std::string& oid, const IParam* p, const int32_t idx) {
            // all we do here is print out the oid of the parameter that was changed
            // your biz logic would do something _even_more_ interesting!
            std::cout << "*** signal received: " << oid << " has been changed by client" << '\n';
        });
    });
    loop.detach();
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // // check that static_root is a valid file path
        // if (!std::filesystem::exists(absl::GetFlag(FLAGS_static_root))) {
        //     std::stringstream why;
        //     why << std::quoted(absl::GetFlag(FLAGS_static_root)) << " is not a valid file path";
        //     throw std::invalid_argument(why.str());
        // }

        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authz = absl::GetFlag(FLAGS_authz);
        CatenaServiceImpl service(cq.get(), dm, EOPath, authz);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms) << '\n';

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        statusUpdate();

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

    // Use an oid to get a pointer to the command you want to define
    // In Catena, commands have IParam type
    std::unique_ptr<IParam> playCommand = dm.getCommand("/play", err);
    assert(playCommand != nullptr);

    // Define a lambda function to be executed when the command is called
    // The lambda function must take a catena::Value as an argument and return a catena::CommandResponse
    playCommand->defineCommand([](catena::Value value) {
        catena::exception_with_status err{"", catena::StatusCode::OK};
        catena::CommandResponse response;
        std::unique_ptr<IParam> stateParam = dm.getParam("/state", err);
        
        // If the state parameter does not exist, return an exception
        if (stateParam == nullptr) {
            response.mutable_exception()->set_type("Invalid Command");
            response.mutable_exception()->set_details(err.what());
            return response;
        }

        std::string& state = dynamic_cast<ParamWithValue<std::string>*>(stateParam.get())->get();
        {
            std::lock_guard lg(dm.mutex());
            state = "playing";
            dm.valueSetByServer.emit("/state", stateParam.get(), 0);
        }
        

        std::cout << "video is " << state << "\n";
        response.mutable_no_response();
        return response;
    });

    std::unique_ptr<IParam> pauseCommand = dm.getCommand("/pause", err);
    assert(pauseCommand != nullptr);
    pauseCommand->defineCommand([](catena::Value value) {
        catena::exception_with_status err{"", catena::StatusCode::OK};
        catena::CommandResponse response;
        std::unique_ptr<IParam> stateParam = dm.getParam("/state", err);

        if (stateParam == nullptr) {
            response.mutable_exception()->set_type("Invalid Command");
            response. mutable_exception()->set_details(err.what());
            return response;
        }

        std::string& state = dynamic_cast<ParamWithValue<std::string>*>(stateParam.get())->get();
        {
            std::lock_guard lg(dm.mutex());
            state = "paused";
            dm.valueSetByServer.emit("/state", stateParam.get(), 0);
        }

        std::cout << "video is " << state << "\n";
        response.mutable_no_response();
        return response;
    });
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