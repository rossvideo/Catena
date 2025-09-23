// Copyright 2025 Ross Video Ltd
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @brief Simple Catena gRPC service example
 * @file web_host.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nathan Rochon (nathan.rochon@rossvideo.com)
 */

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>

// connections/gRPC
#include <ServiceImpl.h>
#include <ServiceCredentials.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <memory>
#include <string>
#include <thread>
#include <signal.h>
#include <fstream>
#include <Logger.h>

#include "httplib.h"  // from cpp-httplib

using grpc::Server;
using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;
std::atomic<bool> running = true;

// Global pointer to web server for control
std::unique_ptr<httplib::Server> globalWebServer;
std::atomic<bool> webServerRunning = false;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        running = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
        if (webServerRunning && globalWebServer) {
            globalWebServer->stop();
        }
    });
    t.join();
}

std::string getExecutableDir(const char* argv0) {
    std::string path(argv0);
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return ".";
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .set_cq(cq.get());
        ServiceImpl service(config);

        builder.RegisterService(&service);

        std::unique_ptr<Server> server(builder.BuildAndStart());
        DEBUG_LOG << "Simple gRPC server listening on " << addr;

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

void RunWebServer(const std::string& exeDir, int port) {
    globalWebServer = std::make_unique<httplib::Server>();
    
    std::string webpage_dir = exeDir + "/webpage/";
    
    // Serve index.html for root
    globalWebServer->Get(R"(/)", [webpage_dir](const httplib::Request&, httplib::Response& res) {
        std::string file_path = webpage_dir + "index.htm";
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            res.set_header("Content-Type", "text/html");
            res.set_content(content, "text/html");
        } 
    });

    // Serve static files from webpage/
    globalWebServer->Get(R"(/(?!api/)(.*))", [webpage_dir](const httplib::Request& req, httplib::Response& res) {
        std::string rel_path = req.matches[1];
        if (rel_path.empty()) return;
        std::string file_path = webpage_dir + rel_path;
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            // Basic content type detection
            if (rel_path.size() >= 5 && rel_path.substr(rel_path.size()-5) == ".html") {
                res.set_header("Content-Type", "text/html");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".css") {
                res.set_header("Content-Type", "text/css");
            } else if (rel_path.size() >= 3 && rel_path.substr(rel_path.size()-3) == ".js") {
                res.set_header("Content-Type", "application/javascript");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".png") {
                res.set_header("Content-Type", "image/png");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".jpg") {
                res.set_header("Content-Type", "image/jpeg");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".ico") {
                res.set_header("Content-Type", "image/x-icon");
            } else {
                res.set_header("Content-Type", "application/octet-stream");
            }
            res.set_content(content, res.get_header_value("Content-Type"));
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
        }
    });
    
    // API endpoint for service status
    globalWebServer->Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        res.set_content(R"({"status": "running", "service": "Visual Audio Deck gRPC Service"})", "application/json");
    });
    
    // API endpoint for service info
    globalWebServer->Get("/api/info", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        res.set_content(R"({"name": "Visual Audio Deck", "type": "Catena gRPC Service", "version": "1.0"})", "application/json");
    });

    DEBUG_LOG << "Starting web server on port " << port;
    webServerRunning = true;
    globalWebServer->listen("0.0.0.0", port);
    webServerRunning = false;
}



int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs a simple Catena gRPC Service with Web Interface");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    // Get executable directory for static file serving
    std::string exeDir = getExecutableDir(argv[0]);

    DEBUG_LOG << "Starting Catena gRPC service with web interface...";
    DEBUG_LOG << "gRPC server will run on: " << addr;
    DEBUG_LOG << "Web server will run on: http://localhost:" << (absl::GetFlag(FLAGS_port) + 1);
    
    // Start web server in its own thread
    std::thread webServerThread(RunWebServer, exeDir, absl::GetFlag(FLAGS_port) + 1);

    // Start the gRPC server in the main thread
    RunRPCServer(addr);

    DEBUG_LOG << "gRPC service shutting down...";

    // Shut down web server after gRPC server ends
    if (webServerRunning && globalWebServer) {
        globalWebServer->stop();
    }
    webServerThread.join();

    DEBUG_LOG << "All services shut down.";

    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}