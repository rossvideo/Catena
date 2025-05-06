/*
 * Copyright 2024 Ross Video Ltd
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
 * @brief Example program to demonstrate setting up a full Catena service.
 * @file status_update.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @author John Danen (john.danen@rossvideo.com)
 */

// device model
#include "device.status_update.json.h" 

//common
#include <utils.h>
#include <Device.h>
#include <ParamWithValue.h>

// connections/gRPC
#include <SharedFlags.h>
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

using grpc::Server;
using catena::gRPC::CatenaServiceImpl;

using namespace catena::common;


Server *globalServer = nullptr;
std::atomic<bool> globalLoop = true;

// handle SIGINT
void handle_signal(int sig) {
    std::thread t([sig]() {
        std::cout << "Caught signal " << sig << ", shutting down" << std::endl;
        globalLoop = false;
        if (globalServer != nullptr) {
            globalServer->Shutdown();
            globalServer = nullptr;
        }
    });
    t.join();
}

void counterUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& counter = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    std::cout << "*** client set counter to " << counter << '\n';
}

void text_boxUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const std::string& text_box = dynamic_cast<const ParamWithValue<std::string>*>(p)->get();
    std::cout << "*** client set text_box to " << text_box << '\n';
}

void buttonUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& button = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    std::cout << "*** client set button to " << button << '\n';
}

void sliderUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& slider = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    std::cout << "*** client set slider to " << slider << '\n';
}

void combo_boxUpdateHandler(const std::string& oid, const IParam* p, const int32_t idx) {
    // all we do here is print out the oid of the parameter that was changed
    // your biz logic would do something _even_more_ interesting!
    const int32_t& combo_box = dynamic_cast<const ParamWithValue<int32_t>*>(p)->get();
    std::cout << "*** client set combo_box to " << combo_box << '\n';
}

void statusUpdateExample(){   
    std::thread loop([]() {
        std::map<std::string, std::function<void(const std::string&, const IParam*, const int32_t)>> handlers;
        handlers["/counter"] = counterUpdateHandler;
        handlers["/text_box"] = text_boxUpdateHandler;
        handlers["/button"] = buttonUpdateHandler;
        handlers["/slider"] = sliderUpdateHandler;
        handlers["/combo_box"] = combo_boxUpdateHandler;

        // this is the "receiving end" of the status update example
        dm.valueSetByClient.connect([&handlers](const std::string& oid, const IParam* p, const int32_t idx) {
            if (handlers.contains(oid)) {
                handlers[oid](oid, p, idx);
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
                std::cout << counter.getOid() << " set to " << counter.get() << '\n';
                dm.valueSetByServer.emit("/counter", &counter, 0);
            }
        }
    });
    loop.detach();
}

void RunRPCServer(std::string addr)
{
    // install signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGKILL, handle_signal);

    try {
        // // check that static_root is a valid file path
        // if (!std::filesystem::exists(absl::GetFlag(FLAGS_static_root))) {
        //     std::stringstream why;
        //     why << std::quoted(absl::GetFlag(FLAGS_static_root)) << " is not a valid file path";
        //     throw std::invalid_argument(why.str());
        // }
        grpc::ServerBuilder builder;
        // set some grpc options
        grpc::EnableDefaultHealthCheckService(true);

        builder.AddListeningPort(addr, catena::gRPC::getServerCredentials());
        std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
        std::string EOPath = absl::GetFlag(FLAGS_static_root);
        bool authz = absl::GetFlag(FLAGS_authz);
        CatenaServiceImpl service(cq.get(), dm, EOPath, authz);

        // Updating device's default max array length.
        dm.set_default_max_length(absl::GetFlag(FLAGS_default_max_array_size));

        builder.RegisterService(&service);


        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "GRPC on " << addr << " secure mode: " << absl::GetFlag(FLAGS_secure_comms) << '\n';

        globalServer = server.get();

        service.init();
        std::thread cq_thread([&]() { service.processEvents(); });

        statusUpdateExample();

        // wait for the server to shutdown and tidy up
        server->Wait();

        cq->Shutdown();
        cq_thread.join();

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
    }
}

int main(int argc, char* argv[])
{
    std::string addr;
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
  
    addr = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  
    std::thread catenaRpcThread(RunRPCServer, addr);
    catenaRpcThread.join();
    return 0;
}