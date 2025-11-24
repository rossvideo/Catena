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
 * @brief Example program to use Simple Dashboard AudioDeck.
 * @file simple_dashboard_audiodeck.cpp
 * @copyright Copyright © 2025 Ross Video Ltd 
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-11-18
 */

// device model
#include "device.simple_dashboard_audiodeck.json.h" 

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
        LOG(INFO) << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void audioDeckUpdateHandler(const std::string& jptr, const IParam* p) {
    Path oid(jptr);
    if (oid.empty()) {
        LOG(INFO) << "*** Whole struct array was updated";
    } else {
        std::size_t index = oid.front_as_index();
        if (index == Path::kEnd) {
            LOG(INFO) << "*** Index is \"-\", new element added to struct array";
        } else {
            LOG(INFO) << "*** audio_deck[" << index << "] was updated";
        }
    }
}

// Simulate some audio meter levels
void inputMeters() {
    while (globalLoop) {
        for (size_t ch = 0; ch < 8; ++ch) {
            try {
                st2138::Value value;
                std::string inputOid = absl::StrFormat("/audio_deck/%d/input", ch);
                std::string gainOid = absl::StrFormat("/audio_deck/%d/gain", ch);

                dm.getValue(gainOid, value); // ensure param exists
                float level = value.float32_value() + (static_cast<float>(rand() % 100) - 50) / 50.0f;

                value.set_float32_value(level);
                dm.setValue(inputOid, value);
            } catch (const std::exception& e) {
                LOG(ERROR) << "Error updating input meter for channel " << ch << ": " << e.what();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // Define the add_channel command
    std::unique_ptr<IParam> addChannel = dm.getCommand("/add_channel", err);
    assert(addChannel != nullptr);

    /* 
     * Define a lambda function to be executed when the command is called
     * The command accepts a string value containing the input name for the channel
     */
    addChannel->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Extract the input name from the command value
            std::string inputName = value.string_value();
            
            if (inputName.empty()) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details("Input name cannot be empty");
            } else {
                LOG(INFO) << "Add Channel command called with input name: " << inputName;
                // Here you can add server-side logic to handle channel creation
                // For example, you could add an entry to the audio_deck struct array
                response.mutable_no_response();
            }
            
            co_return response;
        }(value, respond));
    });
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
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .set_cq(cq.get())
            .add_dm(&dm);
        ServiceImpl service(config);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        LOG(INFO) << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // route client-set updates for specific top-level params to handlers
        std::map<std::string, std::function<void(const std::string&, const IParam*)>> handlers;
        handlers["audio_deck"] = audioDeckUpdateHandler;

        dm.getValueSetByClient().connect([&handlers](const std::string& oid, const IParam* p) {
            //only print if param isnt input meter
            if (std::regex_match(oid, std::regex(R"(^/audio_deck/\d+/input$)"))) {
                return;
            }
            LOG(INFO) << "signal received: " << oid << " has been changed by client";

            Path jptr(oid);
            std::string front = jptr.front_as_string();
            jptr.pop();

            if (handlers.contains(front)) {
                handlers[front](jptr.toString(true), p);
            }
        });

        std::thread businessLogicThread(inputMeters);
        
        // start the heartbeat on the device
        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();

        // wait for the server to shutdown and tidy up
        server->Wait();
        dm.stopHeartbeat();

        businessLogicThread.join();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[])
{
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    Logger::init("dashboard_audiodeck");
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  
    // Commands should be defined before starting the RPC server
    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    
    return 0;
}
