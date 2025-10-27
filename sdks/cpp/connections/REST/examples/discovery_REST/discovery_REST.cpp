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
 * @brief Example program to demonstrate NMos discovery through a Catena device
 * @file discovery_REST.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Christian Twarog
 */

// device model
#include "device.discovery.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <iostream>
#include <cstring>

// REST
#include <ServiceImpl.h>
#include "NmosNode.h"

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
#include <Logger.h>

using namespace catena::common;
using catena::REST::ServiceConfig;
using catena::REST::ServiceImpl;

static ServiceImpl *globalApi = nullptr;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        LOG(INFO) << "Caught signal " << sig << ", shutting down";
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
        // Setting config.
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_port(absl::GetFlag(FLAGS_port))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .add_dm(&dm);
        
        // Creating and running the REST service.
        ServiceImpl api(config);
        globalApi = &api;
        LOG(INFO) << "API Version: " << api.version();
        LOG(INFO) << "REST on 0.0.0.0:" << config.port;
                
        api.run();
    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    Logger::init("discovery_REST");

    NmosNode node("Catena Device", "Catena Node", "A Catena example Node", "discovery_REST");
    node.init();
    
    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    return 0;
}
