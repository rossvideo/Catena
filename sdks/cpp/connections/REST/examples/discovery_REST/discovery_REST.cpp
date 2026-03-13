/*
 * Copyright 2026 Ross Video Ltd
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
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-02-24
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// device model
#include "device.discovery.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <Config.h>
#include <iostream>
#include <cstring>
#include <ConnectionProps.h>

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
#ifndef _WIN32
    signal(SIGKILL, handle_signal);
#endif

    try {
        // Setting config.
        ServiceConfig config = ServiceConfig().add_dm(&dm);
        
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
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger::init("discovery_REST");

    catena::common::ConnectionProps connectionProps(
        ConnectionProtocol::ST2138_REST,        // Configuration
        30000,                                  // Refresh interval in milliseconds
        "discovery_REST",                       // Node name
        "discovery_REST-a4:bb:6d:6a:6f:a3",     // Node ID
        "/connect/connection-props.xml"         // Endpoint
    );

    if (!connectionProps.start()) {
        LOG(WARNING) << "Failed to start connection props server on port " << config::dashboard_port;
    }

    NmosNode node("Catena Device", "Catena Node", "A Catena example Node", "discovery_REST");
    node.init();
    
    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    return 0;
}
