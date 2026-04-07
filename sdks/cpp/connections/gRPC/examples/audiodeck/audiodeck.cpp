// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @brief Example program to demonstrate setting up a full Catena service.
 * @file audiodeck.cpp
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-02-24
 */

// device model
#include "device.AudioDeck.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <Config.h>
#include <ConnectionProps.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

// protobuf interface
#include <interface/service.grpc.pb.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

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
#include <Logger.h>

using grpc::Server;
using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;

using namespace catena::common;

Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        LOG(INFO) << "Caught signal " << sig << ", shutting down";
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}


void audioDeckUpdateHandler(const std::string& jptr, const IParam* p) {
    Path oid(jptr);
    if(oid.empty()){
        LOG(INFO) << "*** Whole struct array was updated";
    } else{
        std::size_t index = oid.front_as_index();
        if (index == Path::kEnd) {
            LOG(INFO) << "*** Index is \"-\", new element added to struct array";
        } else {
            LOG(INFO) << "*** audio_channel[" << index << "] was updated";
        }
    }

}


        

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // // check that static_root is a valid file path
        // if (!std::filesystem::exists(config::static_root)) {
        //     std::stringstream why;
        //     why << std::quoted(config::static_root) << " is not a valid file path";
        //     throw std::invalid_argument(why.str());
        // }

        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig().set_cq(cq.get()).add_dm(&dm);
        ServiceImpl service(config);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        LOG(INFO) << "GRPC on " << addr << " secure mode: " << config::secure_comms;

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        std::map<std::string, std::function<void(const std::string&, const IParam*)>> handlers;
        handlers["audio_deck"] = audioDeckUpdateHandler;

        dm.getValueSetByClient().connect([&handlers](const std::string& oid, const IParam* p) {
            LOG(INFO) << "signal recieved: " << oid << " has been changed by client";

            // make a copy of the path that we can safely pop segments from
            Path jptr(oid); 
            std::string front = jptr.front_as_string();
            jptr.pop();

            if (handlers.contains(front)) {
                handlers[front](jptr.toString(true), p);
            }
        });

        // start the heartbeat on the device
        dm.setHeartbeatParam("/product/version");
        dm.startHeartbeat();

        // wait for the server to shutdown and tidy up
        server->Wait();
        dm.stopHeartbeat();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

int main(int argc, char* argv[])
{
    const auto [exit, code] = config::initConfigVariables(argc, argv);
    if (exit) {
        return code;
    }
    Logger::init("audiodeck");

    catena::common::ConnectionProps connectionProps(
        ConnectionProtocol::ST2138_GRPC,        // Configuration
        30000,                                  // Refresh interval in milliseconds
        "audiodeck",                            // Node name
        "audiodeck-a4:bb:6d:6a:6f:a3",          // Node ID
        "/connect/connection-props.xml"         // Endpoint
    );

    if (!connectionProps.start()) {
        LOG(WARNING) << "Failed to start connection props server on port " << config::dashboard_port;
    }
  
    std::thread catenaRpcThread(RunRPCServer, "0.0.0.0:" + std::to_string(config::port));
    catenaRpcThread.join();
    
    return 0;
}
