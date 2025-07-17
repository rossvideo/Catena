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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @brief Example program containing one of everyhting.
 * @file one_of_everything.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Benjamin.whitten@rossvideo.com
 */

// device model
#include "device.audio_deck.yaml.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>
#include <RangeConstraint.h>

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

using grpc::Server;
using catena::gRPC::CatenaServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // SELECT

    // SOLO
    std::unique_ptr<IParam> solo_cmd = dm.getCommand("/solo", err);
    assert(solo_cmd != nullptr);
    solo_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            // Channel must be specified
            // SOLO_M
            if (value.has_string_value() && value.string_value() == "m") {
                std::lock_guard lg(dm.mutex());
                std::unique_ptr<IParam> solo_ptr = dm.getParam("/solo_m", err);
                if (solo_ptr) {
                    auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(solo_ptr.get());
                    solo.get() = solo.get() ? 0 : 1;
                    dm.getValueSetByServer().emit("/solo_m", &solo);
                }

            // CHANNEL/SOLO
            } else if (value.has_int32_value()) {
                std::string channel_oid = "/channel_list/" + std::to_string(value.int32_value());
                std::lock_guard lg(dm.mutex());
                if (dm.getParam(channel_oid, err)) {
                    // Getting fields solo
                    std::unique_ptr<IParam> solo_ptr = dm.getParam(channel_oid + "/solo", err);
                    std::unique_ptr<IParam> select_ptr = dm.getParam(channel_oid + "/select", err);
                    std::unique_ptr<IParam> slider_ptr = dm.getParam(channel_oid + "/slider", err);
                    std::unique_ptr<IParam> display_ptr = dm.getParam(channel_oid + "/display", err);
                    if (solo_ptr && select_ptr && slider_ptr && display_ptr) {
                        auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(solo_ptr.get());
                        auto& select = *dynamic_cast<ParamWithValue<int32_t>*>(select_ptr.get());
                        auto& slider = *dynamic_cast<ParamWithValue<float>*>(slider_ptr.get());
                        if (select.get() && !solo.get()) {
                            solo.get() = 1;
                            dm.getValueSetByServer().emit("/solo_m", &solo);
                            // Updating display
                            catena::Value newDisplay, file, mode, subtext;
                            file.set_string_value("nice image");
                            mode.set_string_value("SET FREQ");
                            subtext.set_string_value("frequency");
                            newDisplay.mutable_struct_value()->mutable_fields()->insert({"file", file});
                            newDisplay.mutable_struct_value()->mutable_fields()->insert({"mode", mode});
                            newDisplay.mutable_struct_value()->mutable_fields()->insert({"subtext", subtext});
                            display_ptr->fromProto(newDisplay, catena::common::Authorizer::kAuthzDisabled);
                            // Updating slider range.



                        } else if (select.get() && solo.get()) {
                            solo.get() = 0;
                            dm.getValueSetByServer().emit("/solo_m", &solo);
                        } else {
                            solo.get() = solo.get() ? 0 : 1;
                            dm.getValueSetByServer().emit("/solo_m", &solo);
                        }
                    }
                }

            } else {
                err = catena::exception_with_status("Channel number not specified", catena::StatusCode::INVALID_ARGUMENT);
            }
            
            // Sending response to client.
            catena::CommandResponse response;
            if (err.status == catena::StatusCode::OK) {
                response.mutable_no_response();
            } else {
                response.mutable_exception()->set_details(err.what());
            }
            co_return response;
        }(value));
    });

    // MUTE
    std::unique_ptr<IParam> mute_cmd = dm.getCommand("/mute", err);
    assert(mute_cmd != nullptr);
    mute_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::string oid = "";
            // Channel must be specified
            if (value.has_string_value() && value.string_value() == "m") {
                oid = "/mute_m";
            } else if (value.has_int32_value()) {
                oid = "/channel_list/" + std::to_string(value.int32_value()) + "/mute";
            } else {
                err = catena::exception_with_status("Channel number not specified", catena::StatusCode::INVALID_ARGUMENT);
            }
            // Toggling mute
            if (!oid.empty()) {
                std::lock_guard lg(dm.mutex());
                std::unique_ptr<IParam> mute_ptr = dm.getParam(oid, err);
                if (mute_ptr) {
                    auto& mute = *dynamic_cast<ParamWithValue<int32_t>*>(mute_ptr.get());
                    mute.get() = mute.get() ? 0 : 1;
                    dm.getValueSetByServer().emit(oid, &mute);
                }
            }
            // Sending response to client.
            catena::CommandResponse response;
            if (err.status == catena::StatusCode::OK) {
                response.mutable_no_response();
            } else {
                response.mutable_exception()->set_details(err.what());
            }
            co_return response;
        }(value));
    });

    // CLEAR
    std::unique_ptr<IParam> clear_cmd = dm.getCommand("/clear", err);
        assert(clear_cmd != nullptr);
        clear_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
            return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
                catena::exception_with_status err{"", catena::StatusCode::OK};
                // Setting solo_m to false.
                std::lock_guard lg(dm.mutex());
                std::unique_ptr<IParam> solo_ptr = dm.getParam("/solo_m", err);
                if (solo_ptr) {
                    auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(solo_ptr.get());
                    solo.get() = 0;
                    dm.getValueSetByServer().emit("/solo_m", &solo);
                }
                // Continue to set all channel solo to false.
                std::unique_ptr<IParam> channel_list_ptr = dm.getParam("/channel_list", err);
                if (channel_list_ptr) {
                    for (uint32_t i = 0; i < channel_list_ptr->size(); i += 1) {
                        solo_ptr = dm.getParam("/channel_list/" + std::to_string(i) + "/solo", err);
                        if (solo_ptr) {
                            auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(solo_ptr.get());
                            solo.get() = 0;
                            dm.getValueSetByServer().emit("/channel_list/" + std::to_string(i) + "/solo", &solo);
                        }
                    }
                }
                // Sending response to client.
                catena::CommandResponse response;
                if (err.status == catena::StatusCode::OK) {
                    response.mutable_no_response();
                } else {
                    response.mutable_exception()->set_details(err.what());
                }
                co_return response;
            }(value));
        });
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
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authz = absl::GetFlag(FLAGS_authz);
        CatenaServiceImpl service(cq.get(), {&dm}, EOPath, authz);

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

int main(int argc, char* argv[])
{
    Logger::StartLogging(argc, argv);

    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  
    // commands should be defined before starting the RPC server 
    defineCommands();

    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    
    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}