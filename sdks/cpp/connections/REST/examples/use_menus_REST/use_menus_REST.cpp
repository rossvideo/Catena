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
 * @brief Example program to demonstrate setting up a Catena service using menus with the REST API.
 * @file use_menus_REST.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 * @author Ben Mostafa (ben.mostafa@rossvideo.com)
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 */

// device model
#include "device.use_menus.yaml.h" 

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
#include <Logger.h>

using namespace catena::common;
using catena::REST::ServiceConfig;
using catena::REST::CatenaServiceImpl;

CatenaServiceImpl *globalApi = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
        }
    });
    t.join();
}

void statusUpdateExample(){   
    // this is the "receiving end" of the status update example
    dm.getValueSetByClient().connect([](const std::string& oid, const IParam* p) {
        // all we do here is print out the oid of the parameter that was changed
        // your biz logic would do something _even_more_ interesting!
        DEBUG_LOG << "*** signal received: " << oid << " has been changed by client";
    });

    // The rest is the "sending end" of the status update example
    IParam* param = dm.getItem<ParamTag>("counter");
    if (param == nullptr) {
        std::stringstream why;
        why << __PRETTY_FUNCTION__ << "\nparam 'counter' not found";
        throw catena::exception_with_status(why.str(), catena::StatusCode::NOT_FOUND);
    }

    // downcast the IParam to a ParamWithValue<int32_t>
    auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param);

    while (globalLoop) {
        // update the counter once per second, and emit the event
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            std::lock_guard lg(dm.mutex());
            counter.get()++;
            DEBUG_LOG << counter.getOid() << " set to " << counter.get();
            dm.getValueSetByServer().emit("/counter", &counter);
        }
    }
}

void RunRESTServer() {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // Setting config.
        ServiceConfig config;
        config.dms.push_back(&dm);
        config.EOPath = absl::GetFlag(FLAGS_static_root);
        config.authz = absl::GetFlag(FLAGS_authz);
        config.port = absl::GetFlag(FLAGS_port);
        config.maxConnections = absl::GetFlag(FLAGS_max_connections);
        
        // Creating and running the REST service.
        CatenaServiceImpl api(config);
        globalApi = &api;
        DEBUG_LOG << "API Version: " << api.version();
        DEBUG_LOG << "REST on 0.0.0.0:" << config.port;
        
        std::thread counterLoop(statusUpdateExample);

        api.run();

        counterLoop.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    Logger::StartLogging(argc, argv);

    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    
    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
} 