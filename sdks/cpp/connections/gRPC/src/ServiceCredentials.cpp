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

#include <SharedFlags.h>
#include <ServiceCredentials.h>

// catena
#include <utils.h>

#include <ServiceImpl.h> // JWTMetaAuthMetadataProcessor
#include <regex>

using catena::readFile;

// expand env variables
void catena::expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}

// creates a Security Credentials object based on the command line options
std::shared_ptr<grpc::ServerCredentials> catena::getServerCredentials() {
    std::shared_ptr<::grpc::ServerCredentials> ans;
    if (absl::GetFlag(FLAGS_secure_comms).compare("off") == 0) {
        // run without secure comms
        ans = ::grpc::InsecureServerCredentials();
    } else if (absl::GetFlag(FLAGS_secure_comms).compare("tls") == 0) {
        // create our server credentials options
        ::grpc::SslServerCredentialsOptions ssl_opts(
          absl::GetFlag(FLAGS_mutual_authc) ? GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
                                            : GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);

        // get path to the files we'll need
        std::string path_to_certs(absl::GetFlag(FLAGS_certs));
        catena::expandEnvVariables(path_to_certs);

        // read the CA cert if we are using a private CA, use to set ssl options
        if (absl::GetFlag(FLAGS_private_ca)) {
            std::string ca_cert_fn(path_to_certs + "/" + absl::GetFlag(FLAGS_ca_file));
            std::string ca_cert = catena::readFile(ca_cert_fn);
            ssl_opts.pem_root_certs = ca_cert;
        }
        
        // read the server key and cert, use to set ssl options
        std::string server_key_fn(path_to_certs + "/" + absl::GetFlag(FLAGS_key_file));
        std::string server_certfn(path_to_certs + "/" + absl::GetFlag(FLAGS_cert_file));
        std::string server_key = catena::readFile(server_key_fn);
        std::string server_cert = catena::readFile(server_certfn);
        ssl_opts.pem_key_cert_pairs.push_back(
          ::grpc::SslServerCredentialsOptions::PemKeyCertPair{server_key, server_cert});

        // create the credentials object
        ans = ::grpc::SslServerCredentials(ssl_opts);

        // attach the authz processor if needed
        if (absl::GetFlag(FLAGS_authz)) {
            const std::shared_ptr<::grpc::AuthMetadataProcessor> authzProcessor(new JWTAuthMetadataProcessor());
            ans->SetAuthMetadataProcessor(authzProcessor);
        }

    } else {
        std::stringstream why;
        why << std::quoted(absl::GetFlag(FLAGS_secure_comms)) << " is not a valid secure_comms option";
        throw std::invalid_argument(why.str());
    }
    return ans;
}