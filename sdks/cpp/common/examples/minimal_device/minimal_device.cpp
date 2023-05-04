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
 * @brief Reads the device.minimal.json device model and provides read/write
 * access via gRPC.
 *
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @file basic_param_access.cpp
 * @copyright Copyright © 2023, Ross Video Ltd
 */

#include <DeviceModel.h>
#include <Path.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <service.grpc.pb.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using Index = catena::Path::Index;
using DeviceModel =
    typename catena::DeviceModel<catena::Threading::kMultiThreaded>;
using Param = DeviceModel::Param;

ABSL_FLAG(uint16_t, port, 5255, "Catena service port");

class CatenaServiceImpl final : public catena::catena::Service {
  Status GetValue(ServerContext *context, const ::catena::GetValuePayload *req,
                  ::catena::Value *res) override {
    try {
      DeviceModel::Param p = dm_.get().param(req->oid());
      *res = *p.getValue<catena::Value *>();
      std::cout << "GetValue: " << req->oid() << std::endl;
      return Status::OK;

    } catch (catena::exception_with_status &why) {
      std::cerr << why.what() << std::endl;
      return Status(why.status, "GetValue failed", why.what());

    } catch (std::exception &unhandled) {
      std::cerr << "unhandled exception: " << unhandled.what() << std::endl;
      return Status(grpc::StatusCode::INTERNAL, "Unhandled exception",
                    unhandled.what());
    }
  }

  Status SetValue(ServerContext *context, const ::catena::SetValuePayload *req,
                  ::google::protobuf::Empty *res) override {
    try {
      DeviceModel::Param p = dm_.get().param(req->oid());
      if (req->element_index()) {
        p.setValueAt(req->value(), req->element_index());
      } else {
        p.setValue(&req->value());
      }
      std::cout << "SetValue: " << req->oid() << std::endl;
      return Status::OK;

    } catch (catena::exception_with_status& why) {
      std::cerr << why.what() << std::endl;
      return Status(why.status, "SetValue failed", why.what());

    } catch (std::exception& unhandled) {
      std::cerr << "unhandled exception: " << unhandled.what() << std::endl;
      return Status(grpc::StatusCode::INTERNAL, "SetValue failed");
    }

  }

public:
  inline explicit CatenaServiceImpl(DeviceModel &dm)
      : catena::catena::Service{}, dm_{dm} {}

  ~CatenaServiceImpl() {}

private:
  std::reference_wrapper<DeviceModel> dm_;
};

void RunServer(uint16_t port, DeviceModel &dm) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  CatenaServiceImpl service(dm);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  // absl::ParseCommandLine(argc, argv);
  try {
    // read a json file into a DeviceModel object
    DeviceModel dm("../../../example_device_models/device.minimal.json");

    // write the device model to stdout
    std::cout << "Read Device Model: " << dm << '\n';

    RunServer(absl::GetFlag(FLAGS_port), dm);

  } catch (std::exception &why) {
    std::cerr << "Problem: " << why.what() << '\n';
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}