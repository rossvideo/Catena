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
#include <ParamDescriptor.h>

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
#include <Logger.h>

using grpc::Server;
using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;

using namespace catena::common;

Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
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

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .set_cq(cq.get())
            .add_dm(&dm);
        ServiceImpl service(config);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        DEBUG_LOG << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // Notifies the console when a value is set by the client.
        uint32_t valueSetByClientId = dm.getValueSetByClient().connect([](const std::string& oid, const IParam* p) {
            DEBUG_LOG << "*** signal received: " << oid << " has been changed by client";
        });

        // wait for the server to shutdown and tidy up
        server->Wait();
        dm.getValueSetByClient().disconnect(valueSetByClientId);

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
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
    playCommand->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            std::unique_ptr<IParam> stateParam = dm.getParam("/state", err);
            
            // If the state parameter does not exist, return an exception
            if (stateParam == nullptr) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details(err.what());
            } else {
                std::string& state = dynamic_cast<ParamWithValue<std::string>*>(stateParam.get())->get();
                {
                    std::lock_guard lg(dm.mutex());
                    state = "playing";
                    dm.getValueSetByServer().emit("/state", stateParam.get());
                }
                DEBUG_LOG << "video is " << state;
                response.mutable_no_response();
            }
            co_return response;
        }(value, respond));
    });

    std::unique_ptr<IParam> pauseCommand = dm.getCommand("/pause", err);
    assert(pauseCommand != nullptr);
    pauseCommand->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;
            std::unique_ptr<IParam> stateParam = dm.getParam("/state", err);
            
            // If the state parameter does not exist, return an exception
            if (stateParam == nullptr) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details(err.what());
            } else {
                std::string& state = dynamic_cast<ParamWithValue<std::string>*>(stateParam.get())->get();
                {
                    std::lock_guard lg(dm.mutex());
                    state = "paused";
                    dm.getValueSetByServer().emit("/state", stateParam.get());
                }
                DEBUG_LOG << "video is " << state;
                response.mutable_no_response();
            }
            co_return response;
        }(value, respond));
    });

    std::unique_ptr<IParam> debugCounterCommand = dm.getCommand("/debug_counter", err);
    assert(debugCounterCommand != nullptr);
    debugCounterCommand->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;

            // Short-circuit if we are not responding
            if (!respond) co_return response;

            if (!value.has_int32_value()) {
                response.mutable_exception()->set_type("Invalid Argument");
                response.mutable_exception()->set_details("debug_counter command requires an int32 value");
                co_return response;
            }

            if (value.int32_value() <= 0) {
                response.mutable_exception()->set_type("Invalid Argument");
                response.mutable_exception()->set_details("debug_counter command requires a positive int32 value");
                co_return response;
            }

            int32_t counter =  value.int32_value();
            for (int i = 1; i <= counter; i++) {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                response.mutable_response()->set_int32_value(i);

                if (i < counter) {
                    // Yield the response back to the client
                    co_yield response;
                } else {
                    // Last response, use co_return to close stream
                    co_return response;
                }
            }
            
        }(value, respond));
    });

    std::unique_ptr<IParam> multiArgCommand = dm.getCommand("/multi_arg_command", err);
    assert(multiArgCommand != nullptr);
    multiArgCommand->defineCommand([](const catena::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            catena::CommandResponse response;

            video_player::Multi_arg_command commandArgs;
            std::unique_ptr<IParam> multiArgCommand = dm.getCommand("/multi_arg_command", err);
            IParamDescriptor& desc = const_cast<IParamDescriptor&>(multiArgCommand->getDescriptor());
            auto pwv = ParamWithValue<video_player::Multi_arg_command>(commandArgs,  desc);
            pwv.fromProto(value, Authorizer::kAuthzDisabled);

            // Execute command here
            printf("Executed multi arg commnad \nArg1: %d \nArg2: %d\n", commandArgs.arg1, commandArgs.arg2);

            // For now just echo arguments back to client
            *response.mutable_response() = value; 
            co_return response;
            
        }(value, respond));
    }); 
}

int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    // commands should be defined before starting the RPC server 
    defineCommands();
  
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    
    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}