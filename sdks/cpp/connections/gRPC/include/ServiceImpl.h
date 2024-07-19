
#include <common/include/Status.h>
#include <common/include/vdk/signals.h>

#include <lite/include/Device.h>
#include <lite/include/Param.h>

#include <lite/service.grpc.pb.h>

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
using catena::lite::IParam;

class JWTAuthMetadataProcessor : public grpc::AuthMetadataProcessor {
public:
    grpc::Status Process(const InputMetadata& auth_metadata, grpc::AuthContext* context, 
                         OutputMetadata* consumed_auth_metadata, OutputMetadata* response_metadata) override;
};

class CatenaServiceImpl final : public catena::CatenaService::AsyncService {
  public:
    CatenaServiceImpl(ServerCompletionQueue* cq, Device &dm, std::string& EOPath);

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

    using Registry = std::vector<std::unique_ptr<CatenaServiceImpl::CallData>>;
    using RegistryItem = std::unique_ptr<CatenaServiceImpl::CallData>;
    Registry registry_;
    std::mutex registryMutex_;

    //vdk::signal<void()> shutdownSignal;

    ServerCompletionQueue* cq_;
    Device &dm_;
    std::string& EOPath_;

  public:

    void registerItem(CallData *cd);

    void deregisterItem(CallData *cd);

    static std::vector<std::string> getScopes(grpc::ServerContext &context);

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
        bool hasUpdate_{false};
        int objectId_;
        static int objectCounter_;
        unsigned int pushUpdatesId_;
        unsigned int valueSetByClientId_;
        //unsigned int shutdownSignalId_;
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
        catena::PushUpdates res_;
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
        catena::PushUpdates res_;
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