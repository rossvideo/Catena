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
#include <Logger.h>
#include <Authorizer.h>

using namespace catena::common;
using catena::REST::ServiceConfig;
using catena::REST::ServiceImpl;

ServiceImpl *globalApi = nullptr;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        if (globalApi != nullptr) {
            globalApi->Shutdown();
            globalApi = nullptr;
        }
    });
    t.join();
}

void catenaAssetDownloadHandler(const std::string& fqoid, const Authorizer* authz) {
    //insert business logic here
    DEBUG_LOG << "Asset fqoid: " << fqoid << " get operation complete";
}

void catenaAssetUploadHandler(const std::string& fqoid, const Authorizer* authz) {
    //update the assets list
    //insert business logic here
    //TODO: capability to check if asset exists and if has authz before uploading
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> assets = dm.getParam("/assets", err);
    if (assets == nullptr) {
        throw err;
    }

    auto assetsList = dynamic_cast<ParamWithValue<std::vector<std::string>>*>(assets.get());
    if (assetsList == nullptr) {
        throw catena::exception_with_status("assets param is not a list", catena::StatusCode::INVALID_ARGUMENT);
    }

    if (std::find(assetsList->get().begin(), assetsList->get().end(), fqoid) == assetsList->get().end()) {
        assetsList->get().push_back(fqoid);
        //let manager know that the assets list has changed
    }

    DEBUG_LOG << "Asset fqoid: " << fqoid << " upload operation complete";
}

void catenaAssetDeleteHandler(const std::string& fqoid, const Authorizer* authz) {
    //update the assets list
    //insert business logic here
    //TODO: capability to check if asset exists and if has authz before deleting
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> assets = dm.getParam("/assets", err);
    if (assets == nullptr) {
        throw err;
    }

    auto assetsList = dynamic_cast<ParamWithValue<std::vector<std::string>>*>(assets.get());
    if (assetsList == nullptr) {
        throw catena::exception_with_status("assets param is not a list", catena::StatusCode::INVALID_ARGUMENT);
    }

    if (std::find(assetsList->get().begin(), assetsList->get().end(), fqoid) != assetsList->get().end()) {
        assetsList->get().erase(std::remove(assetsList->get().begin(), assetsList->get().end(), fqoid), assetsList->get().end());
        //let manager know that the assets list has changed
    } else {
        throw catena::exception_with_status("Asset not found in the list", catena::StatusCode::NOT_FOUND);
    }

    DEBUG_LOG << "Asset fqoid: " << fqoid << " delete operation complete";
}

void RunRESTServer() {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    // this is the "receiving end" of the asset request example
    dm.getDownloadAssetRequest().connect([](const std::string& fqoid, const Authorizer* authz) {
        try {
            catenaAssetDownloadHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            DEBUG_LOG << "Asset download failed: " << err.what();
        }
    });

    dm.getUploadAssetRequest().connect([](const std::string& fqoid, const Authorizer* authz) {
        try {
            catenaAssetUploadHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            DEBUG_LOG << "Asset upload failed: " << err.what();
        }
    });

    dm.getDeleteAssetRequest().connect([](const std::string& fqoid, const Authorizer* authz) {
        try {
            catenaAssetDeleteHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            DEBUG_LOG << "Asset delete failed: " << err.what();
        }
    });

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
        DEBUG_LOG << "API Version: " << api.version();
        DEBUG_LOG << "REST on 0.0.0.0:" << config.port;
                
        api.run();
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