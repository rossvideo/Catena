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
 * @brief Visual Audio Deck - Catena gRPC service example
 * @file visual_audio_deck.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

// device model
#include "device.visual_audio_deck.yaml.h"

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
std::atomic<bool> running = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        running = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // Define clear_all command
    std::unique_ptr<IParam> clearCommand = dm.getCommand("/clear_all", err);
    assert(clearCommand != nullptr);

    clearCommand->defineCommand([](const st2138::Value& value, const bool respond) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const st2138::Value& value, const bool respond) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            st2138::CommandResponse response;
            
            // Clear all channel solos (INT32 as boolean)
            for (int i = 1; i <= 4; i++) {
                std::string soloOid = "/ch" + std::to_string(i) + "_solo";
                std::unique_ptr<IParam> soloParam = dm.getParam(soloOid, err);
                if (soloParam) {
                    auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                    if (solo) {
                        std::lock_guard lg(dm.mutex());
                        solo->get() = 0; // false
                        dm.getValueSetByServer().emit(soloOid, solo);
                    }
                }
            }
            
            // Clear main solo (INT32 as boolean)
            std::unique_ptr<IParam> mainSoloParam = dm.getParam("/main_solo", err);
            if (mainSoloParam) {
                auto* mainSolo = dynamic_cast<ParamWithValue<int32_t>*>(mainSoloParam.get());
                if (mainSolo) {
                    std::lock_guard lg(dm.mutex());
                    mainSolo->get() = 0; // false
                    dm.getValueSetByServer().emit("/main_solo", mainSolo);
                }
            }
            
            response.mutable_no_response();
            co_return response;
        }(value, respond));
    });
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

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        LOG(ERROR) << "Problem: " << why.what();
    }
}

// Global pointer to web server for control
std::unique_ptr<httplib::Server> globalWebServer;
std::atomic<bool> webServerRunning = false;

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
    
    // API endpoint to get all parameters
    globalWebServer->Get("/api/params", [](const httplib::Request&, httplib::Response& res) {
        catena::exception_with_status err{"", catena::StatusCode::OK};
        
        std::stringstream json;
        json << std::fixed << std::setprecision(6);
        json << "{\n";
        json << "  \"main\": {\n";
        
        // Main output
        std::unique_ptr<IParam> mainOutputParam = dm.getParam("/main_output", err);
        if (mainOutputParam) {
            auto* mainOutput = dynamic_cast<ParamWithValue<float>*>(mainOutputParam.get());
            if (mainOutput) {
                json << "    \"output\": " << mainOutput->get() << ",\n";
            }
        }
        
        // Main slider
        std::unique_ptr<IParam> mainSliderParam = dm.getParam("/main_slider", err);
        if (mainSliderParam) {
            auto* mainSlider = dynamic_cast<ParamWithValue<float>*>(mainSliderParam.get());
            if (mainSlider) {
                json << "    \"slider\": " << mainSlider->get() << ",\n";
            }
        }
        
        // Main select (INT32 as boolean)
        std::unique_ptr<IParam> mainSelectParam = dm.getParam("/main_select", err);
        if (mainSelectParam) {
            auto* mainSelect = dynamic_cast<ParamWithValue<int32_t>*>(mainSelectParam.get());
            if (mainSelect) {
                json << "    \"select\": " << (mainSelect->get() != 0 ? "true" : "false") << ",\n";
            }
        }
        
        // Main solo (INT32 as boolean)
        std::unique_ptr<IParam> mainSoloParam = dm.getParam("/main_solo", err);
        if (mainSoloParam) {
            auto* mainSolo = dynamic_cast<ParamWithValue<int32_t>*>(mainSoloParam.get());
            if (mainSolo) {
                json << "    \"solo\": " << (mainSolo->get() != 0 ? "true" : "false") << ",\n";
            }
        }
        
        // Main mute (INT32 as boolean)
        std::unique_ptr<IParam> mainMuteParam = dm.getParam("/main_mute", err);
        if (mainMuteParam) {
            auto* mainMute = dynamic_cast<ParamWithValue<int32_t>*>(mainMuteParam.get());
            if (mainMute) {
                json << "    \"mute\": " << (mainMute->get() != 0 ? "true" : "false") << "\n";
            }
        }
        
        json << "  },\n";
        json << "  \"channels\": [\n";
        
        for (int i = 1; i <= 4; i++) {
            json << "    {\n";
            json << "      \"id\": " << i << ",\n";
            
            // Channel signal
            std::string signalOid = "/ch" + std::to_string(i) + "_signal";
            std::unique_ptr<IParam> signalParam = dm.getParam(signalOid, err);
            if (signalParam) {
                auto* signal = dynamic_cast<ParamWithValue<float>*>(signalParam.get());
                if (signal) {
                    json << "      \"signal\": " << signal->get() << ",\n";
                }
            }
            
            // Channel volume
            std::string volumeOid = "/ch" + std::to_string(i) + "_volume";
            std::unique_ptr<IParam> volumeParam = dm.getParam(volumeOid, err);
            if (volumeParam) {
                auto* volume = dynamic_cast<ParamWithValue<float>*>(volumeParam.get());
                if (volume) {
                    json << "      \"volume\": " << volume->get() << ",\n";
                }
            }
            
            // Channel frequency
            std::string freqOid = "/ch" + std::to_string(i) + "_frequency";
            std::unique_ptr<IParam> freqParam = dm.getParam(freqOid, err);
            if (freqParam) {
                auto* freq = dynamic_cast<ParamWithValue<float>*>(freqParam.get());
                if (freq) {
                    json << "      \"frequency\": " << freq->get() << ",\n";
                }
            }
            
            // Channel alpha
            std::string alphaOid = "/ch" + std::to_string(i) + "_alpha";
            std::unique_ptr<IParam> alphaParam = dm.getParam(alphaOid, err);
            if (alphaParam) {
                auto* alpha = dynamic_cast<ParamWithValue<float>*>(alphaParam.get());
                if (alpha) {
                    json << "      \"alpha\": " << alpha->get() << ",\n";
                }
            }
            
            // Channel select (INT32 as boolean)
            std::string selectOid = "/ch" + std::to_string(i) + "_select";
            std::unique_ptr<IParam> selectParam = dm.getParam(selectOid, err);
            if (selectParam) {
                auto* select = dynamic_cast<ParamWithValue<int32_t>*>(selectParam.get());
                if (select) {
                    json << "      \"select\": " << (select->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel solo (INT32 as boolean)
            std::string soloOid = "/ch" + std::to_string(i) + "_solo";
            std::unique_ptr<IParam> soloParam = dm.getParam(soloOid, err);
            if (soloParam) {
                auto* solo = dynamic_cast<ParamWithValue<int32_t>*>(soloParam.get());
                if (solo) {
                    json << "      \"solo\": " << (solo->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel mute (INT32 as boolean)
            std::string muteOid = "/ch" + std::to_string(i) + "_mute";
            std::unique_ptr<IParam> muteParam = dm.getParam(muteOid, err);
            if (muteParam) {
                auto* mute = dynamic_cast<ParamWithValue<int32_t>*>(muteParam.get());
                if (mute) {
                    json << "      \"mute\": " << (mute->get() != 0 ? "true" : "false") << ",\n";
                }
            }
            
            // Channel slider
            std::string sliderOid = "/ch" + std::to_string(i) + "_slider";
            std::unique_ptr<IParam> sliderParam = dm.getParam(sliderOid, err);
            if (sliderParam) {
                auto* slider = dynamic_cast<ParamWithValue<float>*>(sliderParam.get());
                if (slider) {
                    json << "      \"slider\": " << slider->get() << "\n";
                }
            }
            
            json << "    }";
            if (i < 4) json << ",";
            json << "\n";
        }
        
        json << "  ]\n";
        json << "}";
        
        res.set_header("Content-Type", "application/json");
        res.set_content(json.str(), "application/json");
    });
    
    webServerRunning = true;
    globalWebServer->listen("0.0.0.0", port);
    webServerRunning = false;
}

int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs the Visual Audio Deck Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));

    // Get executable directory for static file serving
    std::string exeDir = getExecutableDir(argv[0]);
    defineCommands();

    DEBUG_LOG << "Starting Visual Audio Deck Catena gRPC service with web interface...";
    DEBUG_LOG << "gRPC server will run on: " << addr;
    DEBUG_LOG << "Web server will run on: http://localhost:" << (absl::GetFlag(FLAGS_port) + 1);
    
    // Start web server in its own thread
    std::thread webServerThread(RunWebServer, exeDir, absl::GetFlag(FLAGS_port) + 1);

    // Start Catena RPC server in its own thread
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();

    // Shut down web server after Catena RPC thread ends
    if (webServerRunning && globalWebServer) {
        globalWebServer->stop();
    }
    webServerThread.join();

    DEBUG_LOG << "All services shut down.";

    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}