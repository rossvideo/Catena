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
bool signalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        DEBUG_LOG << "Caught signal " << sig << ", shutting down";
        signalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void updateDisplay(IParam* display_ptr, const std::string& file_str, const std::string& mode_str, const std::string& subtext_str) {
    catena::Value newDisplay, file, mode, subtext;
    file.set_string_value(file_str);
    mode.set_string_value(mode_str);
    subtext.set_string_value(subtext_str);
    newDisplay.mutable_struct_value()->mutable_fields()->insert({"file", file});
    newDisplay.mutable_struct_value()->mutable_fields()->insert({"mode", mode});
    newDisplay.mutable_struct_value()->mutable_fields()->insert({"subtext", subtext});
    display_ptr->fromProto(newDisplay, catena::common::Authorizer::kAuthzDisabled);
}

void defineCommands() {
    catena::exception_with_status err{"", catena::StatusCode::OK};

    // ========================================================================
    //                              SELECT
    // ========================================================================
    std::unique_ptr<IParam> select_cmd = dm.getCommand("/select", err);
    assert(select_cmd != nullptr);
    select_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::lock_guard lg(dm.mutex());
            // Fields
            auto& select_m = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/select_m", err).get());
            std::unique_ptr<IParam> display_m = dm.getParam("/display", err);
            std::unique_ptr<IParam> channel_list_ptr = dm.getParam("/channel_list", err);
            /* 
             * SELECT_M
             * - Sets select_m to TRUE
             * - Updates display_m
             * - Sets all channel/select to FALSE
             * - Sets all channel/slider to their channel/data/volume
             * - Updates all channel/display
             */
            if (value.has_string_value() && value.string_value() == "m") {
                if (!select_m.get()) {
                    select_m.get() = 1;
                    updateDisplay(display_m.get(), "Ross Video Icon", "LR", "main");
                    for (uint32_t i = 0; i < channel_list_ptr->size(); i += 1) {
                        std::string channel_oid = "/channel_list/" + std::to_string(i);
                        auto& select = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam(channel_oid + "/select", err).get());
                        auto& volume = *dynamic_cast<ParamWithValue<float>*>(dm.getParam(channel_oid + "/data/volume", err).get());
                        auto& slider = *dynamic_cast<ParamWithValue<float>*>(dm.getParam(channel_oid + "/slider", err).get());
                        std::unique_ptr<IParam> display_m = dm.getParam(channel_oid + "/display", err);
                        select.get() = 0;
                        slider.get() = volume.get();
                        updateDisplay(dm.getParam(channel_oid + "/display", err).get(), "volume img", std::to_string(i), std::to_string(volume.get()));
                    }
                }
            }
            /*
             * CHANNEL SELECT
             * - Sets channel/select to TRUE
             * - Sets all channel/slider to frequency
             * - Updates all channel/display
             * - Sets select_m to FALSE
             * - Updates display_m
             */
            else if (value.has_int32_value()) {
                std::unique_ptr<IParam> select_ptr = dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/select", err);
                std::unique_ptr<IParam> channel = dm.getParam("/channel_list/" + std::to_string(value.int32_value()), err);
                std::unique_ptr<IParam> channel_list = dm.getParam("/channel_list", err);
                if (channel && select_ptr) {
                    auto& select = *dynamic_cast<ParamWithValue<int32_t>*>(select_ptr.get());
                    select.get() = 1;

                    for (uint32_t i = 0; i < channel_list->size(); i += 1) {
                        auto& slider = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/channel_list/" + std::to_string(i) + "/slider", err).get());
                        std::unique_ptr<IParam> display = dm.getParam("/channel_list/" + std::to_string(i) + "/display", err);
                        updateDisplay(display.get(), "img", "Freq", "frequency");
                    }

                    select_m.get() = 0;
                    updateDisplay(display_m.get(), "sinwave", std::to_string(value.int32_value()), channel->getDescriptor().name().at("en"));
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

    // ========================================================================
    //                               SOLO
    // ========================================================================
    std::unique_ptr<IParam> solo_cmd = dm.getCommand("/solo", err);
    assert(solo_cmd != nullptr);
    solo_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::lock_guard lg(dm.mutex());
            /*
             * SOLO_M
             * - Toggles solo_m
             */
            if (value.has_string_value() && value.string_value() == "m") {
                auto& solo_m = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/solo_m", err).get());
                solo_m.get() = solo_m.get() ? 0 : 1;
            }
            /*
             * CHANNEL_SOLO
             * - Toggles channel/solo
             * - if channel/selected is TRUE
             *   - if channel/solo is TRUE
             *     - Updates channel/display
             *     - Updates channel/slider range constraint
             *   - if channel/solo is FALSE
             *     - Saves frequency
             */
            else if (value.has_int32_value()) {
                std::unique_ptr<IParam> solo_ptr = dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/solo", err);
                if (solo_ptr) {
                    auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(solo_ptr.get());
                    auto& select = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/select", err).get());
                    auto& slider = *dynamic_cast<ParamWithValue<float>*>(dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/slider", err).get());
                    std::unique_ptr<IParam> display = dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/display", err);

                    if (select.get() && !solo.get()) {
                        updateDisplay(display.get(), "img", "SET FREQ", "frequency");
                        slider.defineConstraint(dm.getItem<catena::common::ConstraintTag>("/selected_slider_constraint"));
                    } else if (select.get() && solo.get()) {
                        // Save frequency
                        slider.defineConstraint(dm.getItem<catena::common::ConstraintTag>("/slider_constraint"));
                    }

                    // Toggling solo
                    solo.get() = solo.get() ? 0 : 1;
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

    // ========================================================================
    //                               MUTE
    // ========================================================================
    std::unique_ptr<IParam> mute_cmd = dm.getCommand("/mute", err);
    assert(mute_cmd != nullptr);
    mute_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::lock_guard lg(dm.mutex());
            /*
             * SELECT_M
             * - Toggles mute_m
             */
            if (value.has_string_value() && value.string_value() == "m") {
                auto& mute_m = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/mute_m", err).get());
                mute_m.get() = mute_m.get() ? 0 : 1;
            }
            /*
             * CHANNEL MUTE
             * - Toggles channel/mute
             */
            else if (value.has_int32_value()) {
                std::unique_ptr<IParam> mute_ptr = dm.getParam("/channel_list/" + std::to_string(value.int32_value()) + "/mute", err);
                if (mute_ptr) {
                    auto& mute = *dynamic_cast<ParamWithValue<int32_t>*>(mute_ptr.get());
                    mute.get() = mute.get() ? 0 : 1;
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

    // ========================================================================
    //                               CLEAR
    // ========================================================================
    std::unique_ptr<IParam> clear_cmd = dm.getCommand("/clear", err);
    assert(clear_cmd != nullptr);
    clear_cmd->defineCommand([](const catena::Value& value) -> std::unique_ptr<IParamDescriptor::ICommandResponder> {
        return std::make_unique<ParamDescriptor::CommandResponder>([](const catena::Value& value) -> ParamDescriptor::CommandResponder {
            catena::exception_with_status err{"", catena::StatusCode::OK};
            std::lock_guard lg(dm.mutex());

            /*
             * CLEAR
             * - Sets all channel/solo to FALSE
             */
            std::unique_ptr<IParam> channel_list = dm.getParam("/channel_list", err);
            for (uint32_t i = 0; i < channel_list->size(); i += 1) {
                auto& solo = *dynamic_cast<ParamWithValue<int32_t>*>(dm.getParam("/channel_list/" + std::to_string(i) + "/solo", err).get());
                solo.get() = 0;
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

void runSignal(IParam* signal_ptr) {
    auto& signal = *dynamic_cast<ParamWithValue<int32_t>*>(signal_ptr);
    while (signalLoop) {
        // EQUATION???
        std::this_thread::sleep_for(std::chrono::milliseconds(17));
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

        // Start signal threads
        std::vector<std::thread> signalThreads;
        // catena::exception_with_status err{"", catena::StatusCode::OK};
        // std::unique_ptr<IParam> channel_list = dm.getParam("/channel_list", err);
        // assert(channel_list != nullptr);
        // for (uint32_t i = 0; i < channel_list->size(); i += 1) {
        //     signalThreads.emplace_back(runSignal, dm.getParam("/channel_list/" + std::to_string(i) + "/data/signal", err).get());
        // }

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

        // For for signal threads to join before shutting down.
        for (auto& thread : signalThreads) {
            thread.join();
        }

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