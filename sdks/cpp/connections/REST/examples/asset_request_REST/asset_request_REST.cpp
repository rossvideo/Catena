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
 * @brief Example program to demonstrate Catena asset request with the REST API.
 * @file asset_request_REST.cpp
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-02-24
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// device model
#include "device.asset_request.json.h" 

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
#include <IAuthorizer.h>

using namespace catena::common;
using catena::REST::ServiceConfig;
using catena::REST::ServiceImpl;

ServiceImpl *globalApi = nullptr;

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

void catenaAssetDownloadHandler(const std::string& fqoid, const IAuthorizer* authz) {
    //insert business logic here
    LOG(INFO) << "Asset fqoid: " << fqoid << " get operation complete";
}

void catenaAssetUploadHandler(const std::string& fqoid, const IAuthorizer* authz) {
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

    LOG(INFO) << "Asset fqoid: " << fqoid << " upload operation complete";
}

void catenaAssetDeleteHandler(const std::string& fqoid, const IAuthorizer* authz) {
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

    LOG(INFO) << "Asset fqoid: " << fqoid << " delete operation complete";
}

void RunRESTServer() {
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    // this is the "receiving end" of the asset request example
    dm.getDownloadAssetRequest().connect([](const std::string& fqoid, const IAuthorizer* authz) {
        try {
            catenaAssetDownloadHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            LOG(INFO) << "Asset download failed: " << err.what();
        }
    });

    dm.getUploadAssetRequest().connect([](const std::string& fqoid, const IAuthorizer* authz) {
        try {
            catenaAssetUploadHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            LOG(INFO) << "Asset upload failed: " << err.what();
        }
    });

    dm.getDeleteAssetRequest().connect([](const std::string& fqoid, const IAuthorizer* authz) {
        try {
            catenaAssetDeleteHandler(fqoid, authz);
        } catch (catena::exception_with_status& err) {
            LOG(INFO) << "Asset delete failed: " << err.what();
        }
    });

    try {
        // Setting config.
        ServiceConfig config = ServiceConfig().add_dm(&dm);
        
        // Creating and running the REST service.
        ServiceImpl api(config);
        globalApi = &api;
        LOG(INFO) << "API Version: " << api.version();
        LOG(INFO) << "REST on 0.0.0.0:" << config.port;

        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();
        api.run();
        dm.stopHeartbeat();
    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[]) {
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger::init("asset_request_REST");

    catena::common::ConnectionProps connectionProps(
        ConnectionProtocol::ST2138_REST,        // Configuration
        30000,                                  // Refresh interval in milliseconds
        "asset_request_REST",                   // Node name
        "asset_request_REST-a4:bb:6d:6a:6f:a3", // Node ID
        "/connect/connection-props.xml"         // Endpoint
    );

    if (!connectionProps.start()) {
        LOG(WARNING) << "Failed to start connection props server on port " << config::dashboard_port;
    }

    std::thread catenaRestThread(RunRESTServer);
    catenaRestThread.join();
    
    return 0;
}
