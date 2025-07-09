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
 * @brief Example program to demonstrate setting up a full Catena service with
 * the REST API.
 * @file status_update_REST.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 * @author Ben Whitten (Benjamin.whitten@rossvideo.com)
 */

// device model
#include "device.status_update.yaml.h" 

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

catena::REST::CatenaServiceImpl *globalApi = nullptr;
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

void counterUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& counter = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set counter to " << counter;
}

void text_boxUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const std::string& text_box = dynamic_cast<const ParamWithValue<std::string>*>(p)->get();
    DEBUG_LOG << "*** client set text_box to " << text_box;
}

void buttonUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& button = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set button to " << button;
}

void sliderUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& slider = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set slider to " << slider;
}

void combo_boxUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& combo_box = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    DEBUG_LOG << "*** client set combo_box to " << combo_box;
}

void statusUpdateExample(){   
    std::map<std::string, std::function<void(const std::string&, const IParam*)>> handlers;
    handlers["/counter"] = counterUpdateHandler;
    handlers["/text_box"] = text_boxUpdateHandler;
    handlers["/button"] = buttonUpdateHandler;
    handlers["/slider"] = sliderUpdateHandler;
    handlers["/combo_box"] = combo_boxUpdateHandler;

    // this is the "receiving end" of the status update example
    dm.valueSetByClient.connect([&handlers](const std::string& oid, const IParam* p) {
        if (handlers.contains(oid)) {
            handlers[oid](oid, p);
        }
    });

    catena::exception_with_status err{"", catena::StatusCode::OK};

    // The rest is the "sending end" of the status update example
    std::unique_ptr<IParam> param = dm.getParam("/counter", err);
    if (param == nullptr) {
        throw err;
    }

    // downcast the IParam to a ParamWithValue<int32_t>
    auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param.get());

    while (globalLoop) {
        // update the counter once per second, and emit the event
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            std::lock_guard lg(dm.mutex());
            counter.get()++;
            DEBUG_LOG << counter.getOid() << " set to " << counter.get();
            dm.valueSetByServer.emit("/counter", &counter);
        }
    }
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
        catena::REST::CatenaServiceImpl api({&dm}, EOPath, authorization, port);
        globalApi = &api;
        DEBUG_LOG << "API Version: " << api.version();
        DEBUG_LOG << "REST on 0.0.0.0:" << port;
        
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
