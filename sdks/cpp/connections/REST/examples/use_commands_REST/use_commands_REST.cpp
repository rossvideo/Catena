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
 * @brief Example program to demonstrate using commands with the REST API.
 * @file use_commands_REST.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author John Danen (john.danen@rossvideo.com)
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 */

// device model
#include "device.video_player.yaml.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>

// REST
#include <ServiceImpl.h>

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

using namespace catena::common;

catena::REST::CatenaServiceImpl *globalApi = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        globalLoop = false;
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
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

void RunRESTServer() {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // Getting flags.
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authorization = absl::GetFlag(FLAGS_authz);
        uint16_t port = absl::GetFlag(FLAGS_port);
        
        // Creating and running the REST service.
        catena::REST::CatenaServiceImpl api(dm, EOPath, authorization, port);
        globalApi = &api;
        std::cout << "API Version: " << api.version() << std::endl;
        std::cout << "REST on 0.0.0.0:" << port << std::endl;
        
        statusUpdate();
        
        api.run();
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
            response.mutable_exception()->set_details(err.what());
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

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    
    // commands should be defined before starting the REST server 
    defineCommands();
    
    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    return 0;
} 