#pragma once

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
 * @file ServiceImpl.h
 * @brief Implements Catena gRPC request handlers
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// common
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>

//lite 
#include <Device.h>

// gRPC interface
#include <interface/service.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <jwt-cpp/jwt.h>

#include <condition_variable>
#include <chrono>

using grpc::ServerContext;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;
using grpc::ServerCompletionQueue;

using catena::lite::Device;
using catena::common::IParam;

class JWTAuthMetadataProcessor : public grpc::AuthMetadataProcessor {
public:
    grpc::Status Process(const InputMetadata& auth_metadata, grpc::AuthContext* context, 
                         OutputMetadata* consumed_auth_metadata, OutputMetadata* response_metadata) override;
};

class CatenaServiceImpl final : public catena::CatenaService::AsyncService {
  public:
    CatenaServiceImpl(ServerCompletionQueue* cq, Device &dm, std::string& EOPath, bool authz);

    void init();

    void processEvents();

    void shutdownServer();

    /**
     * Nested private classes
     */
    enum class CallStatus { kCreate, kProcess, kWrite, kPostWrite, kFinish };

  private:
    /**
     * @brief Abstract base class for the CallData classes
     * Provides a the proceed method
     */
    class CallData {
      public:
        virtual void proceed(CatenaServiceImpl *service, bool ok) = 0;
        virtual ~CallData() {}
    };

    std::vector<std::string> getScopes(grpc::ServerContext &context);

    using Registry = std::vector<std::unique_ptr<CatenaServiceImpl::CallData>>;
    using RegistryItem = std::unique_ptr<CatenaServiceImpl::CallData>;
    Registry registry_;
    std::mutex registryMutex_;

    ServerCompletionQueue* cq_;
    Device &dm_;
    std::string& EOPath_;
    bool authz_;

  public:

    void registerItem(CallData *cd);

    void deregisterItem(CallData *cd);

    class GetPopulatedSlots : public CallData{
        public:
        GetPopulatedSlots(CatenaServiceImpl *service, Device &dm, bool ok);

        void proceed(CatenaServiceImpl *service, bool ok) override;

       private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        google::protobuf::Empty req_;
        catena::SlotList res_;
        ServerAsyncResponseWriter<::catena::SlotList> responder_;
        CallStatus status_;
        Device &dm_;
        int objectId_;
        static int objectCounter_;
    };

    class GetValue : public CallData{
        public:
        GetValue(CatenaServiceImpl *service, Device &dm, bool ok);

        void proceed(CatenaServiceImpl *service, bool ok) override;

       private:

        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::GetValuePayload req_;
        catena::Value res_;
        ServerAsyncResponseWriter<::catena::Value> responder_;
        CallStatus status_;
        Device &dm_;
        int objectId_;
        static int objectCounter_;
    };

    /**
     * @brief CallData class for the SetValue RPC
     */
    class SetValue : public CallData {
      public:
        SetValue(CatenaServiceImpl *service, Device &dm, bool ok);

        void proceed(CatenaServiceImpl *service, bool ok) override;

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::SetValuePayload req_;
        catena::Value res_;
        ServerAsyncResponseWriter<::google::protobuf::Empty> responder_;
        CallStatus status_;
        Device &dm_;
        Status errorStatus_;
        int objectId_;
        static int objectCounter_;
    };

    /**
     * @brief CallData class for the Connect RPC
     */
    class Connect : public CallData {
      public:
        Connect(CatenaServiceImpl *service, Device &dm, bool ok);

        void proceed(CatenaServiceImpl *service, bool ok) override;

     private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::ConnectPayload req_;
        catena::PushUpdates res_;
        ServerAsyncWriter<catena::PushUpdates> writer_;
        CallStatus status_;
        Device &dm_;
        std::mutex mtx_;
        std::condition_variable cv_;
        std::vector<std::string> clientScopes_;
        bool hasUpdate_{false};
        int objectId_;
        static int objectCounter_;
        unsigned int pushUpdatesId_;
        unsigned int valueSetByClientId_;
        unsigned int valueSetByServerId_;
        static vdk::signal<void()> shutdownSignal_;
        unsigned int shutdownSignalId_;
    };

    /**
     * @brief CallData class for the DeviceRequest RPC
     */
    class DeviceRequest : public CallData {
      public:
        DeviceRequest(CatenaServiceImpl *service, Device &dm, bool ok);

        void proceed(CatenaServiceImpl *service, bool ok) override;

       private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        std::vector<std::string> clientScopes_;
        catena::DeviceRequestPayload req_;
        ServerAsyncWriter<catena::DeviceComponent> writer_;
        CallStatus status_;
        Device &dm_;
        int objectId_;
        static int objectCounter_;
        unsigned int shutdownSignalId_;
    };

    class ExternalObjectRequest : public CallData {
      public:
        ExternalObjectRequest(CatenaServiceImpl *service, Device &dm, bool ok);
        ~ExternalObjectRequest() {}

        void proceed(CatenaServiceImpl *service, bool ok) override;

      private:
        CatenaServiceImpl *service_;
        ServerContext context_;
        catena::ExternalObjectRequestPayload req_;
        ServerAsyncWriter<catena::ExternalObjectPayload> writer_;
        CallStatus status_;
        Device &dm_;
        int objectId_;
        static int objectCounter_;
    };

    // /**
    //  * @brief CallData class for the GetParam RPC
    //  */
    // class GetParam : public CallData {
    //   public:
    //     GetParam(CatenaServiceImpl *service, Device &dm, bool ok);
    //     ~GetParam() {}

    //     void proceed(CatenaServiceImpl *service, bool ok) override;

    //   private:
    //     CatenaServiceImpl *service_;
    //     ServerContext context_;
    //     std::vector<std::string> clientScopes_;
    //     catena::GetParamPayload req_;
    //     catena::PushUpdates res_;
    //     ServerAsyncWriter<catena::DeviceComponent_ComponentParam> writer_;
    //     CallStatus status_;
    //     Device &dm_;
    //     std::unique_ptr<catena::ParamAccessor> param_;
    //     int objectId_;
    //     static int objectCounter_;
    // };


};