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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @brief host a web page that acts as hardware that is controlled via Catena
 * @file web_host.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nathan Rochon (nathan.rochon@rossvideo.com)
 */

// device model
#include "device.web_host.yaml.h"

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

#include "httplib.h"  // from cpp-httplib

using grpc::Server;
using catena::gRPC::ServiceConfig;
using catena::gRPC::ServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;
std::atomic<bool> fibLoop = false;
std::unique_ptr<std::thread> fibThread = nullptr;
std::atomic<bool> counterLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        fibLoop = false;
        counterLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void startCounter() {
    catena::exception_with_status err{"", catena::StatusCode::OK};
    // The rest is the "sending end" of the status update example
    std::unique_ptr<IParam> param = dm.getParam("/counter", err);
    if (param == nullptr) {
        throw err;
    }
    // downcast the IParam to a ParamWithValue<int32_t>
    auto& counter = *dynamic_cast<ParamWithValue<int32_t>*>(param.get());
    counter.get() = 0; // Initialize counter to 0
    while (counterLoop) {
        // update the counter once per second, and emit the event
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            std::lock_guard lg(dm.mutex());
            if (counter.get()++ >= 200) {
                counter.get() = 0; // Reset counter to 0 when it reaches 200
            }
            DEBUG_LOG << counter.getOid() << " set to " << counter.get();
            dm.getValueSetByServer().emit("/counter", &counter);
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
        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        ServiceConfig config = ServiceConfig()
            .set_EOPath(absl::GetFlag(FLAGS_static_root))
            .set_authz(absl::GetFlag(FLAGS_authz))
            .set_maxConnections(absl::GetFlag(FLAGS_max_connections))
            .set_cq(cq.get())
            .add_dm(&dm);
        ServiceImpl service(config);

        // Updating device's default max array length.
        dm.set_default_max_length(absl::GetFlag(FLAGS_default_max_array_size));

        builder.RegisterService(&service);


        std::unique_ptr<Server> server(builder.BuildAndStart());
        DEBUG_LOG << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms);

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        std::thread loop(startCounter);

        // wait for the server to shutdown and tidy up
        server->Wait();

        if (fibThread) { fibThread->join(); }
        loop.join();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}


// Global pointer to web server for control
std::unique_ptr<httplib::Server> globalWebServer;
std::atomic<bool> webServerRunning = false;

std::string getExecutableDir(const char* argv0) {
    std::string path(argv0);
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return ".";
}
void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // Use an oid to get a pointer to the command you want to define
    // In Catena, commands have IParam type
    std::unique_ptr<IParam> defaultCommand = dm.getCommand("/default", err);
    assert(defaultCommand != nullptr);

    // Define a lambda function to be executed when the command is called
    // The lambda function must take a st2138::Value as an argument and return a catena::CommandResponse
    defaultCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            std::unique_ptr<IParam> sliderParam = dm.getParam("/slider", err);

            // If the slider parameter does not exist, return an exception
            if (sliderParam == nullptr) {
                response.mutable_exception()->set_type("Invalid Command");
                response.mutable_exception()->set_details(err.what());
            } else {
                std::float_t& sliderValue = dynamic_cast<ParamWithValue<std::float_t>*>(sliderParam.get())->get();
                {
                    std::lock_guard lg(dm.mutex());
                    sliderValue = 50.0f;
                    dm.getValueSetByServer().emit("/slider", sliderParam.get());
                }
                DEBUG_LOG << "slider is " << sliderValue;
                response.mutable_no_response();
            }
            co_return response;
        }(value, respond));
    });
    }

void RunBusinessLogic(const std::string& exeDir,int port){

    globalWebServer = std::make_unique<httplib::Server>();
 
    std::string webpage_dir = exeDir + "/webpage/";
    // Serve index.htm for root
    globalWebServer->Get(R"(/)", [webpage_dir](const httplib::Request&, httplib::Response& res) {
        std::string file_path = webpage_dir + "index.htm";
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            res.set_header("Content-Type", "text/html");
            res.set_content(content, "text/html");
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
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
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".ico") {
                res.set_header("Content-Type", "image/x-icon");
            } else if (rel_path.size() >= 4 && rel_path.substr(rel_path.size()-4) == ".png") {
                res.set_header("Content-Type", "image/png");
            } else {
                res.set_header("Content-Type", "application/octet-stream");
            }
            res.set_content(content, res.get_header_value("Content-Type"));
        } else {
            res.status = 404;
            res.set_content("404 Not Found", "text/plain");
        }
    });
    globalWebServer->Get("/api/slider-stream", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider(
            "text/event-stream",
            [](size_t /*offset*/, httplib::DataSink &sink) {
                while (counterLoop) {
                    catena::exception_with_status err{"", catena::StatusCode::OK};
                    std::unique_ptr<IParam> param = dm.getParam("/slider", err);
                    float value = 0.0f;
                    if (param) {
                        auto* slider = dynamic_cast<ParamWithValue<float_t>*>(param.get());
                        if (slider) {
                            value = slider->get();
                        }
                    }
                    std::string sse = "data: " + std::to_string(value) + "\n\n";
                    if (!sink.is_writable()) break;
                    sink.write(sse.data(), sse.size());
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                sink.done();
                return true;
            }
        );
    });
    // Endpoint to set slider value
    globalWebServer->Post("/api/slider", [](const httplib::Request& req, httplib::Response& res) {
        auto valStr = req.body;
        try {
            float value = std::stof(valStr);
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::unique_ptr<IParam> param = dm.getParam("/slider", err);
            if (param) {
                auto* slider = dynamic_cast<ParamWithValue<float_t>*>(param.get());
                if (slider) {
                    slider->get() = value;
                    dm.getValueSetByServer().emit("/slider", param.get());
                    DEBUG_LOG << slider->getOid() << " set to " << slider->get();
                    res.set_content("OK", "text/plain");
                    return;
                }
            }
            res.status = 400;
            res.set_content("Param not found or wrong type", "text/plain");
        } catch (...) {
            res.status = 400;
            res.set_content("Invalid value", "text/plain");
        }
    });
    // SSE endpoint for counter updates
    globalWebServer->Get("/api/counter-stream", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider(
            "text/event-stream",
            [](size_t /*offset*/, httplib::DataSink &sink) {
                while (counterLoop) {
                    catena::exception_with_status err{"", catena::StatusCode::OK};
                    std::unique_ptr<IParam> param = dm.getParam("/counter", err);
                    int32_t value = 0;
                    if (param) {
                        auto* counter = dynamic_cast<ParamWithValue<int32_t>*>(param.get());
                        if (counter) {
                            value = counter->get();
                        }
                    }
                    std::string sse = "data: " + std::to_string(value) + "\n\n";
                    if (!sink.is_writable()) break;
                    sink.write(sse.data(), sse.size());
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                sink.done();
                return true;
            }
        );
    });
    // Endpoint to set slider to default value
    globalWebServer->Post("/api/set_default", [](const httplib::Request& req, httplib::Response& res) {
        catena::exception_with_status err{"", catena::StatusCode::OK};
        std::unique_ptr<IParam> param = dm.getParam("/slider", err);
        if (param) {
            auto* slider = dynamic_cast<ParamWithValue<float_t>*>(param.get());
            if (slider) {
                slider->get() = 50.0f;
                dm.getValueSetByServer().emit("/slider", param.get());
                DEBUG_LOG << slider->getOid() << " set to " << slider->get();
                res.set_content("OK", "text/plain");
                return;
            }
        }
        res.status = 400;
        res.set_content("Param not found or wrong type", "text/plain");
    });
    webServerRunning = true;
    globalWebServer->listen("0.0.0.0", port);
    webServerRunning = false;
}

int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    // Get executable directory for static file serving
    std::string exeDir = getExecutableDir(argv[0]);
    defineCommands();
    // Start web server in its own thread
    std::thread webServerThread(RunBusinessLogic, exeDir, absl::GetFlag(FLAGS_port) + 1);

    // Start Catena RPC server in its own thread
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    // Shut down web server after Catena RPC thread ends
    if (webServerRunning && globalWebServer) {
        globalWebServer->stop();
    }
    webServerThread.join();

    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}