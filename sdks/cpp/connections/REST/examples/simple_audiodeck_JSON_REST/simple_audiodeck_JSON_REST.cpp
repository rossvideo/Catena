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
 * @brief Example program to demonstrate setting up a full Catena service using REST api.
 * @file simple_audiodeck_JSON_REST.cpp
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-11
 */

// device model
#include "device.simple_audiodeck.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>
#include <Config.h>
#include <ConnectionProps.h>

// REST
#include <ServiceImpl.h>

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
using catena::REST::ServiceImpl;

ServiceImpl *globalApi = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        LOG(INFO) << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
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

void inputMeters() {
    while (globalLoop) {
        for (size_t ch = 0; ch < 8; ++ch) {
            try {
                st2138::Value value;
                std::string inputOid = absl::StrFormat("/audio_deck/%d/input", ch);
                std::string gainOid = absl::StrFormat("/audio_deck/%d/gain", ch);

                dm.getValue(gainOid, value);
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

    std::unique_ptr<IParam> addChannel = dm.getCommand("/add_channel", err);
    assert(addChannel != nullptr);

    addChannel->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> { 
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            std::string inputName = value.string_value();
            
            if (inputName.empty()) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details("Input name cannot be empty");
            } else {
                LOG(INFO) << "Add Channel command called with input name: " << inputName;
                response.mutable_no_response();
            }
            
            co_return response;
        }(value, respond));
    });
}

void RunRESTServer() {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        ServiceConfig config = ServiceConfig().add_dm(&dm);
        
        ServiceImpl api(config);
        globalApi = &api;
        LOG(INFO) << "API Version: " << api.version();
        LOG(INFO) << "REST on 0.0.0.0:" << config.port;

        std::map<std::string, std::function<void(const std::string&, const IParam*)>> handlers;
        handlers["audio_deck"] = audioDeckUpdateHandler;

        dm.getValueSetByClient().connect([&handlers](const std::string& oid, const IParam* p) {
            static const std::regex inputMeterPattern(R"(^/audio_deck/\d+/input$)");
            if (std::regex_match(oid, inputMeterPattern)) {
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

        try {
            dm.setHeartbeatParam("/product/version");
            dm.startHeartbeat();
            api.run();
            dm.stopHeartbeat();
        } catch (...) {
            globalLoop = false;
            businessLogicThread.join();
            throw;
        }

        businessLogicThread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger::init("simple_audiodeck_JSON_REST");

    defineCommands();

    catena::common::ConnectionProps connectionProps(
        ConnectionProtocol::ST2138_REST,
        30000,
        "simple_audiodeck_JSON_REST",
        "simple_audiodeck_JSON_REST-a4:bb:6d:6a:6f:a3",
        "/connect/connection-props.xml"
    );

    if (!connectionProps.start()) {
        LOG(WARNING) << "Failed to start connection props server on port " << config::dashboard_port;
    }

    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    return 0;
}
