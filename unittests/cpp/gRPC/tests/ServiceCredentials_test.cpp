/*
 * Copyright 2026 Ross Video Ltd
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
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-03-20
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

#include "MockDevice.h"
#include "MockConnect.h"
#include "MockServiceImpl.h"
#include "MockAuthContext.h"

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

// Fixture
class gRPCServiceCredentialsTests : public testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        config::log_dir = UNITTEST_LOG_DIR;
        config::log_file = true;
        config::log_level = "TRACE";
        config::log_size = 10;
        config::log_count = 128;
        Logger::init("gRPCServiceCredentialsTest");
    }

    static void TearDownTestSuite() {
    }

    void SetUp() override {
        // Set environment variable for expansion
        setenv("TEST_CERT_PATH", "/tmp/testcerts", 1);

        // Set flags for credential logic
        config::secure_comms = "tls";
        config::certs = "/tmp/testcerts";
        config::cert_file = "server.crt";
        config::key_file = "server.key";

        // Initialize metadata containers
        input_metadata_.clear();
    }

    void TearDown() override {
        // Unset environment variables
        unsetenv("TEST_CERT_PATH");
        // Reset flags
        config::secure_comms = "off";
        config::certs = "";
        config::cert_file = "";
        config::key_file = "";

        // Clear metadata containers
        input_metadata_.clear();
    }

    // Metadata variables for tests
    grpc::AuthMetadataProcessor::InputMetadata input_metadata_;
    JWTAuthMetadataProcessor processor_;
    MockAuthContext mockAuthcontext_;
};

/*
 * TEST 1 -  Normal case for ServiceCredentials Process().
 */
TEST_F(gRPCServiceCredentialsTests, ValidCredentials) {
    grpc::AuthMetadataProcessor::OutputMetadata consumed, response;
    // Fetching JWT token
    std::string token("Bearer " + getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    grpc::string_ref token_ref(token);
    
    // Adds the token to input metadata
    input_metadata_.emplace("authorization", token_ref);
    EXPECT_CALL(mockAuthcontext_, AddProperty("claims", ::testing::_)).Times(1);
    grpc::Status status = processor_.Process(input_metadata_, &mockAuthcontext_, &consumed, &response);

    // Asserts that the status is OK
    EXPECT_EQ(status.error_code(), grpc::StatusCode::OK);
}

/*
 * TEST 2 - ServiceCredentials with missing Authorization metadata.
 */
TEST_F(gRPCServiceCredentialsTests, InvalidCredentialsAuthz) {
    grpc::AuthMetadataProcessor::OutputMetadata consumed, response; 

    grpc::Status status = processor_.Process(input_metadata_, nullptr, &consumed, &response);
    
    // Setting expectations
    EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(status.error_message(), "No bearer token provided");
}

/*
 * TEST 3 - ServiceCredentials with invalid token in Authorization metadata.
 */
TEST_F(gRPCServiceCredentialsTests, InvalidTokenInAuthzMetadata) {
    grpc::AuthMetadataProcessor::OutputMetadata consumed, response;
    input_metadata_.emplace("authorization", "Bearer not_a_valid_jwt_token");

    grpc::Status status = processor_.Process(input_metadata_, nullptr, &consumed, &response);
    
    // Setting expectations
    EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(status.error_message(), "Invalid bearer token");
}

/*
 * TEST 4 - Test with a string containing environment variables (e.g. ${HOME}) and verify replacement.
 */
TEST(gRPCExpandEnvVariablesTest, ReplaceExistingEnvVar) {
    setenv("HOME", "/tmp/home", 1);
    std::string input = "Path: ${HOME}/data";
    catena::gRPC::expandEnvVariables(input);
    
    // Setting expectations
    EXPECT_EQ(input, "Path: /tmp/home/data");
    unsetenv("HOME");
}

/*
 * TEST 5 - Test with a string containing an env variable that doesn't exist.
 */
TEST(gRPCExpandEnvVariablesTest, ReplaceNonExistingEnvVar) {
    unsetenv("DOES_NOT_EXIST");
    std::string input = "Path: ${DOES_NOT_EXIST}/data";
    catena::gRPC::expandEnvVariables(input);
    
    // Setting expectations
    EXPECT_EQ(input, "Path: /data");
}

/*
 * TEST 6 - Test with a string containing no env variables.
 */
TEST(gRPCExpandEnvVariablesTest, NoEnvVarInString) {
    std::string input = "Path: /static/data";
    catena::gRPC::expandEnvVariables(input);
    
    // Setting expectations
    EXPECT_EQ(input, "Path: /static/data");
}

/*
 * TEST 7 - Test with config::secure_comms set to off
 */
TEST_F(gRPCServiceCredentialsTests, FlagsSecureCommsOff) {
    config::secure_comms = "off";

    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_EQ(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 8 - Test with config::secure_comms set to tls and config::mutual_authc set to false.
 */
TEST_F(gRPCServiceCredentialsTests, MutualAuthcFalse) {
    config::secure_comms = "tls";
    config::mutual_authc = false;

    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 9 - Test with config::secure_comms set to tls and config::mutual_authc set to true.
 */
TEST_F(gRPCServiceCredentialsTests, MutualAuthcTrue) {
    config::secure_comms = "tls";
    config::mutual_authc = true;

    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 10 - Test with config::secure_comms set to tls and config::private_ca set to false.
 */
TEST_F(gRPCServiceCredentialsTests, PrivateCaFalse) {
    config::secure_comms = "tls";
    config::private_ca = false;
    config::certs = "/tmp/testcerts";
    config::ca_file = "ca.crt";
    config::key_file = "server.key";
    config::cert_file = "server.crt";

    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 11 - Test with config::secure_comms set to tls and config::private_ca set to true.
 */
TEST_F(gRPCServiceCredentialsTests, PrivateCaTrue) {
    config::secure_comms = "tls";
    config::private_ca = true;
    config::certs = "/tmp/testcerts";
    config::ca_file = "ca.crt";
    config::key_file = "server.key";
    config::cert_file = "server.crt";

    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 12 - Test with config::secure_comms set to tls and config::authz set to false.
 */
TEST_F(gRPCServiceCredentialsTests, AuthzFalse) {
    config::secure_comms = "tls";
    config::authz = false;
    config::certs = "/tmp/testcerts";
    config::key_file = "server.key";
    config::cert_file = "server.crt";
    
    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 13 - Test with config::secure_comms set to tls and config::authz set to true.
 */
TEST_F(gRPCServiceCredentialsTests, AuthzTrue) {
    config::secure_comms = "tls";
    config::private_ca = true;
    config::certs = "/tmp/testcerts";
    config::ca_file = "ca.crt";
    config::key_file = "server.key";
    config::cert_file = "server.crt";
    
    // Setting expectations
    auto creds = catena::gRPC::getServerCredentials();
    auto insecure = grpc::InsecureServerCredentials();
    EXPECT_NE(creds, nullptr);
    EXPECT_NE(typeid(*creds), typeid(*insecure));
}

/*
 * TEST 14 - Test with config::secure_comms set to an invalid value.
 */
TEST_F(gRPCServiceCredentialsTests, SecureCommsInvalidValue) {
    config::secure_comms = "invalid_value";

    // Setting expectations
    EXPECT_THROW(catena::gRPC::getServerCredentials(), std::invalid_argument);
}
