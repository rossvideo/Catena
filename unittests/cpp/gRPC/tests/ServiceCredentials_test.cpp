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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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
 * @file ServiceCredentials_test.cpp
 * @brief This file is for testing the gRPC ServiceCredentials.cpp file.
 * @author Jason Chen (jason.chen@rossvideo.com)
 * @date 2025-11-03
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

#include "MockDevice.h"
#include "MockConnect.h"
#include "MockServiceImpl.h"

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include "ServiceImpl.h"
#include <ServiceCredentials.h>
#include <jwt-cpp/jwt.h>

// common
#include <Status.h>
#include <Logger.h>

// Test helpers
#include "GRPCTest.h"
#include "CommonTestHelpers.h"
#include "MockParam.h"

using namespace catena::common;
using namespace catena::gRPC;

// Define the log directory for unit tests
#define UNITTEST_LOG_DIR "/tmp/unittest_logs"

// Fixture
class gRPCServiceCredentialsTests : public testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("gRPCServiceCredentialsTest");
    }

    static void TearDownTestSuite() {
    }

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout_ = std::cout.rdbuf(MockConsole_.rdbuf());
        EXPECT_CALL(dm_, slot()).WillRepeatedly(testing::Return(0));

        // Set environment variable for expansion
        setenv("TEST_CERT_PATH", "/tmp/testcerts", 1);

        // Set flags for credential logic
        absl::SetFlag(&FLAGS_secure_comms, "tls");
        absl::SetFlag(&FLAGS_certs, "/tmp/testcerts");
        absl::SetFlag(&FLAGS_key_file, "server.key");
        absl::SetFlag(&FLAGS_cert_file, "server.crt");

        // Initialize metadata containers
        input_metadata.clear();
        output_metadata.clear();
    }

    void TearDown() override {
        // Unset environment variables
        unsetenv("TEST_CERT_PATH");
        // Reset flags
        absl::SetFlag(&FLAGS_secure_comms, "off");
        absl::SetFlag(&FLAGS_certs, "");
        absl::SetFlag(&FLAGS_key_file, "");
        absl::SetFlag(&FLAGS_cert_file, "");
        // Remove mock files
        // Clear metadata containers
        input_metadata.clear();
        output_metadata.clear();
    }
    
    // Cout variables
    std::stringstream MockConsole_;
    std::streambuf* oldCout_;

    // Address used for gRPC tests.
    std::string serverAddr_ = "0.0.0.0:50051";
    // Metadata variables for tests
    using InputMetadata = std::multimap<grpc::string_ref, grpc::string_ref>;
    using OutputMetadata = std::multimap<std::string, std::string>;
    InputMetadata input_metadata;
    OutputMetadata output_metadata;
    JWTAuthMetadataProcessor processor;
    grpc::ClientContext clientContext_;
    // Server and service variables.
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_ = nullptr;
    std::unique_ptr<ServiceImpl> service_ = nullptr;
    MockDevice dm_;
    // Random variables
    bool authzEnabled_ = false;
};

// TEST_F(gRPCServiceCredentialsTests, ValidCredentials) {
//     JWTAuthMetadataProcessor processor;
//     OutputMetadata consumed, response; 
//     // Adding authorization mockToken metadata.
//     authzEnabled_ = true;

//     // Generate a valid JWT token
//     std::string valid_token = jwt::create()
//         .set_payload_claim("sub", jwt::claim(std::string("testuser")))
//         .sign(jwt::algorithm::hs256{"claims"});
//     // Adds the token to input metadata
//     input_metadata.emplace("authorization", "Bearer " + valid_token);
//     grpc::Status status = processor.Process(input_metadata, nullptr, &consumed, &response);
//     // Asserts that the status is OK
//     EXPECT_EQ(status.error_code(), grpc::StatusCode::OK);
// }

TEST_F(gRPCServiceCredentialsTests, InvalidCredentialsAuthz) {
    OutputMetadata consumed, response; 

    grpc::Status status = processor.Process(input_metadata, nullptr, &consumed, &response);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(status.error_message(), "No bearer token provided");
}

TEST_F(gRPCServiceCredentialsTests, InvalidTokenInAuthzMetadata) {
    OutputMetadata consumed, response;
    input_metadata.emplace("authorization", "Bearer not_a_valid_jwt_token");

    grpc::Status status = processor.Process(input_metadata, nullptr, &consumed, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(status.error_message(), "Invalid bearer token");
}

TEST(gRPCExpandEnvVariablesTest, ReplaceExistingEnvVar) {
    setenv("HOME", "/tmp/home", 1);
    std::string input = "Path: ${HOME}/data";
    catena::gRPC::expandEnvVariables(input);
    EXPECT_EQ(input, "Path: /tmp/home/data");
    unsetenv("HOME");
}

TEST(gRPCExpandEnvVariablesTest, ReplaceNonExistingEnvVar) {
    unsetenv("DOES_NOT_EXIST");
    std::string input = "Path: ${DOES_NOT_EXIST}/data";
    catena::gRPC::expandEnvVariables(input);
    EXPECT_EQ(input, "Path: /data");
}

TEST(gRPCExpandEnvVariablesTest, NoEnvVarInString) {
    std::string input = "Path: /static/data";
    catena::gRPC::expandEnvVariables(input);
    EXPECT_EQ(input, "Path: /static/data");
}

TEST_F(gRPCServiceCredentialsTests, FlagsSecureCommsOff) {
    absl::SetFlag(&FLAGS_secure_comms, "off");

    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_EQ(typeid(*creds), typeid(*insecure));
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_mutual_authcFalse) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_mutual_authc, false);

    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(typeid(*creds), typeid(*insecure));
    EXPECT_NE(creds, nullptr);
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_mutual_authcTrue) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_mutual_authc, true);

    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(typeid(*creds), typeid(*insecure));
    EXPECT_NE(creds, nullptr);
}

// New test cases for FLAGS_private_ca
TEST_F(gRPCServiceCredentialsTests, FLAGS_private_caFalse) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_private_ca, false);
    absl::SetFlag(&FLAGS_certs, "/tmp/testcerts");
    absl::SetFlag(&FLAGS_ca_file, "ca.crt");
    absl::SetFlag(&FLAGS_key_file, "server.key");
    absl::SetFlag(&FLAGS_cert_file, "server.crt");

    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(typeid(*creds), typeid(*insecure));
    EXPECT_NE(creds, nullptr);
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_private_caTrue) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_private_ca, true);
    absl::SetFlag(&FLAGS_certs, "/tmp/testcerts");
    absl::SetFlag(&FLAGS_ca_file, "ca.crt");
    absl::SetFlag(&FLAGS_key_file, "server.key");
    absl::SetFlag(&FLAGS_cert_file, "server.crt");

    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(typeid(*creds), typeid(*insecure));
    EXPECT_NE(creds, nullptr);
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_authzFalse) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_authz, false);
    absl::SetFlag(&FLAGS_certs, "/tmp/testcerts");
    absl::SetFlag(&FLAGS_key_file, "server.key");
    absl::SetFlag(&FLAGS_cert_file, "server.crt");

    auto creds = catena::gRPC::getServerCredentials();
    EXPECT_NO_THROW(creds->SetAuthMetadataProcessor(nullptr));
    EXPECT_NE(creds, nullptr);
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_authzTrue) {
    absl::SetFlag(&FLAGS_secure_comms, "tls");
    absl::SetFlag(&FLAGS_authz, true);
    absl::SetFlag(&FLAGS_certs, "/tmp/testcerts");
    absl::SetFlag(&FLAGS_key_file, "server.key");
    absl::SetFlag(&FLAGS_cert_file, "server.crt");

    auto creds = catena::gRPC::getServerCredentials();
    EXPECT_NO_THROW(creds->SetAuthMetadataProcessor(nullptr));
    EXPECT_NE(creds, nullptr);
}

TEST_F(gRPCServiceCredentialsTests, FLAGS_secure_commsInvalidValue) {
    absl::SetFlag(&FLAGS_secure_comms, "invalid_value");
    EXPECT_THROW(catena::gRPC::getServerCredentials(), std::invalid_argument);
}