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

/**
 * @brief Example program to demonstrate Catena external object request
 * the REST API.
 * @file external_object_request_REST.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Christian Twarog
 */

// device model
#include "device.asset_request.json.h" 

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

#include <iostream>

using namespace catena::common;

catena::REST::CatenaServiceImpl *globalApi = nullptr;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
        }
    });
    t.join();
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
                
        api.run();
    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    
    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    return 0;
}