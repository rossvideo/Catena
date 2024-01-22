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
#include <ParamAccessor.h>
#include <Path.h>
#include <utils.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <service.grpc.pb.h>

#include <jwt-cpp/jwt.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using Index = catena::Path::Index;
using DeviceModel = typename catena::DeviceModel<catena::Threading::kMultiThreaded>;
using ParamAccessor = typename catena::ParamAccessor<DeviceModel>;

// set up the command line parameters
ABSL_FLAG(uint16_t, port, 6254, "Catena service port");
ABSL_FLAG(std::string, certs, "${HOME}/test_certs", "path/to/certs/files");
ABSL_FLAG(std::string, secure_comms, "off", "Specify type of secure comms, options are: \
  \"off\", \"ssl\", \"tls\"");
ABSL_FLAG(bool, mutual_authc, false, "use this to require client to authenticate");
ABSL_FLAG(bool, authz, false, "use OAuth token authorization");
ABSL_FLAG(std::string, device_model, "../../../example_device_models/device.minimal.json", "Specify the JSON device model to use.");

// expand env variables
void expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}

// creates a Security Credentials object based on the command line options
std::shared_ptr<grpc::ServerCredentials> getServerCredentials() {
    std::shared_ptr<grpc::ServerCredentials> ans;
    if (absl::GetFlag(FLAGS_secure_comms).compare("off") == 0) {
        // run without secure comms
        ans = grpc::InsecureServerCredentials();
    } else if (absl::GetFlag(FLAGS_secure_comms).compare("ssl") == 0) {
        // run with Secure Sockets Layer comms
        std::string path_to_certs(absl::GetFlag(FLAGS_certs));
        expandEnvVariables(path_to_certs);
        std::string root_cert = catena::readFile(path_to_certs + "/ca.crt");
        std::string server_key = catena::readFile(path_to_certs + "/server.key");
        std::string server_cert = catena::readFile(path_to_certs + "/server.crt");
        grpc::SslServerCredentialsOptions ssl_opts(
          absl::GetFlag(FLAGS_mutual_authc) ? GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
                                            : GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);
        ssl_opts.pem_root_certs = root_cert;
        ssl_opts.pem_key_cert_pairs.push_back(
          grpc::SslServerCredentialsOptions::PemKeyCertPair{server_key, server_cert});
        ans = grpc::SslServerCredentials(ssl_opts);
    } else if (absl::GetFlag(FLAGS_secure_comms).compare("tls") == 0) {
        std::stringstream why;
        why << "tls support has not been implemented yet, sorry.";
        throw std::runtime_error(why.str());
    } else {
        std::stringstream why;
        why << std::quoted(absl::GetFlag(FLAGS_secure_comms)) << " is not a valid secure_comms option";
        throw std::invalid_argument(why.str());
    }
    return ans;
}

// if authz is enabled, inspect the bearer token to decide whether to grant
// access.
//
// right now, it just tests that a token exists, decodes it and prints it out
// much work required to actually validate the token
void authorize(grpc::ServerContext *context /*catena::Param &p*/) {
    if (absl::GetFlag(FLAGS_authz)) {
        auto authz = context->client_metadata();
        auto it = authz.find("authorization");
        if (it == authz.end()) {
            BAD_STATUS("No authorization token found", catena::StatusCode::PERMISSION_DENIED);
        }

        // remove the 'Bearer ' text from the beginning
        grpc::string_ref t = it->second.substr(7);
        std::string token(t.begin(), t.end());
        std::cout << "authz: " << token << '\n';
        auto decoded = jwt::decode(token);
        for (auto &e : decoded.get_payload_json()) {
            std::cout << e.first << ": " << e.second << '\n';
        }
    }
}

class CatenaServiceImpl final : public catena::CatenaService::Service {
    Status GetValue(ServerContext *context, const ::catena::GetValuePayload *req,
                    ::catena::Value *res) override {
        try {
            ParamAccessor p = dm_.get().param(req->oid());
            authorize(context);
            *res = p.getValue<catena::Value>(req->element_index());
            std::cout << "GetValue: " << req->oid() << std::endl;
            return Status::OK;

        } catch (catena::exception_with_status &why) {
            std::cerr << why.what() << std::endl;
            grpc::StatusCode gs = static_cast<grpc::StatusCode>(why.status);
            return Status(gs, "GetValue failed", why.what());

        } catch (...) {
            std::cerr << "unhandled exception: " << std::endl;
            return Status(grpc::StatusCode::INTERNAL, "Unhandled exception");
        }
    }

    Status SetValue(ServerContext *context, const ::catena::SetValuePayload *req,
                    ::google::protobuf::Empty *res) override {
        try {
            auto p = dm_.get().param(req->oid());
            authorize(context);
            p.setValue(req->value(), req->element_index());
            std::cout << "SetValue: " << req->oid() << ", " << req->element_index() << '\n';
            return Status::OK;

        } catch (catena::exception_with_status &why) {
            std::cerr << why.what() << std::endl;
            grpc::StatusCode gs = static_cast<grpc::StatusCode>(why.status);
            return Status(gs, "SetValue failed", why.what());

        } catch (...) {
            std::cerr << "unhandled exception: " << std::endl;
            return Status(grpc::StatusCode::INTERNAL, "SetValue failed");
        }
    }

  public:
    inline explicit CatenaServiceImpl(DeviceModel &dm) : catena::CatenaService::Service{}, dm_{dm} {}

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
    builder.AddListeningPort(server_address, getServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << " with secure comms "
              << (absl::GetFlag(FLAGS_secure_comms).compare("off") == 0 ? "disabled" : "enabled") << '\n';

    // Wait for the server to shutdown. Note that some other thread
    // must be responsible for shutting down the server for this call
    // to ever return.
    server->Wait();
}

int main(int argc, char **argv) {
    absl::SetProgramUsageMessage("Runs the Catena Service");
    absl::ParseCommandLine(argc, argv);
    std::string path(absl::GetFlag(FLAGS_certs));
    expandEnvVariables(path);
    std::cout << path << '\n';
    try {
        // read a json file into a DeviceModel object
        DeviceModel dm(absl::GetFlag(FLAGS_device_model));

        RunServer(absl::GetFlag(FLAGS_port), dm);

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}