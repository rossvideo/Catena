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
 * @brief Example program to demonstrate setting up a full Catena service with
 * the REST API.
 * @file status_update_REST.cpp
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 * @author Ben Whitten (Benjamin.whitten@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-02-24
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// device model
#include "device.status_update.yaml.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <Config.h>
#include <ConnectionProps.h>

// REST
#include <ServiceImpl.h>
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

#include <boost/log/trivial.hpp>

using namespace catena::common;
using catena::REST::ServiceConfig;
using catena::REST::ServiceImpl;

ServiceImpl *globalApi = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        BOOST_LOG_TRIVIAL(info) << "Caught signal " << sig << ", shutting down";
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
    BOOST_LOG_TRIVIAL(info) << "*** client set counter to " << counter;
}

void text_boxUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const std::string& text_box = dynamic_cast<const ParamWithValue<std::string>*>(p)->get();
    BOOST_LOG_TRIVIAL(info) << "*** client set text_box to " << text_box;
}

void buttonUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& button = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    BOOST_LOG_TRIVIAL(info) << "*** client set button to " << button;
}

void sliderUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& slider = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    BOOST_LOG_TRIVIAL(info) << "*** client set slider to " << slider;
}

void combo_boxUpdateHandler(const std::string& oid, const IParam* p) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& combo_box = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    BOOST_LOG_TRIVIAL(info) << "*** client set combo_box to " << combo_box;
}

void statusUpdateExample(){   
    std::map<std::string, std::function<void(const std::string&, const IParam*)>> handlers;
    handlers["/counter"] = counterUpdateHandler;
    handlers["/text_box"] = text_boxUpdateHandler;
    handlers["/button"] = buttonUpdateHandler;
    handlers["/slider"] = sliderUpdateHandler;
    handlers["/combo_box"] = combo_boxUpdateHandler;

    // this is the "receiving end" of the status update example
    dm.getValueSetByClient().connect([&handlers](const std::string& oid, const IParam* p) {
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
            // VLOG(1) << counter.getOid() << " set to " << counter.get();
            BOOST_LOG_TRIVIAL(info) << counter.getOid() << " set to " << counter.get();
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
        ServiceConfig config = ServiceConfig().add_dm(&dm);
        
        // Creating and running the REST service.
        ServiceImpl api(config);
        globalApi = &api;
        BOOST_LOG_TRIVIAL(info) << "API Version: " << api.version();
        BOOST_LOG_TRIVIAL(info) << "REST on 0.0.0.0:" << config.port;
        
        std::thread counterLoop(statusUpdateExample);

        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();
        api.run();
        dm.stopHeartbeat();

        counterLoop.join();
    } catch (std::exception &why) {
        BOOST_LOG_TRIVIAL(error) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger2::init("status_update_REST");

    // catena::common::ConnectionProps connectionProps(
    //     ConnectionProtocol::ST2138_REST,        // Configuration
    //     30000,                                  // Refresh interval in milliseconds
    //     "status_update_REST",                   // Node name
    //     "status_update_REST-a4:bb:6d:6a:6f:a3", // Node ID
    //     "/connect/connection-props.xml"         // Endpoint
    // );

    // if (!connectionProps.start()) {
    //     BOOST_LOG_TRIVIAL(warning) << "Failed to start connection props server on port " << config::dashboard_port;
    // }

    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    return 0;
}
