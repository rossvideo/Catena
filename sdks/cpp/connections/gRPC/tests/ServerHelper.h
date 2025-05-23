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
 * @brief This file containing a class with helpful functions to simulate a
 * client and server.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/22
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// protobuf
#include <grpcpp/grpcpp.h>
#include "../../common/tests/CommonMockClasses.h"
#include "gRPCMockClasses.h"

class ServerHelper {
  public:
    // Creates a server.
    void createServer() {
        // Creating the server.
        builder.AddListeningPort(serverAddr, grpc::InsecureServerCredentials());
        cq = builder.AddCompletionQueue();
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        std::cout<<"Server created"<<std::endl;
    }

    // Creates a client.
    void createClient() {
        // Creating the client.
        channel = grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials());
        stub = catena::CatenaService::NewStub(channel);
        std::cout<<"Client created"<<std::endl;
    }

    void shutdown() {
        // Cleaning up the server.
        server->Shutdown();
        cq->Shutdown();
        // Draining the cq
        void* ignored_tag;
        bool ignored_ok;
        while (cq->Next(&ignored_tag, &ignored_ok)) {}
        // Deleting the test and async calls if they exist.
        if (testCall) { delete testCall; }
        if (asyncCall) { delete asyncCall; }
    }

  protected:
    std::string serverAddr = "0.0.0.0:50051";
    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server = nullptr;
    MockServiceImpl service;
    std::unique_ptr<grpc::ServerCompletionQueue> cq = nullptr;
    std::shared_ptr<grpc::Channel> channel = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> stub = nullptr;
    std::mutex mtx;
    MockDevice dm;
    ICallData* testCall = nullptr;
    ICallData* asyncCall = nullptr;
};
