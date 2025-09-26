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
 * @brief This file is for testing the ExternalObjectRequest.cpp file.
 * @author nelson.daniels@rossvideo.com
 * @date 25/09/19
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"
#include "StreamReader.h"

// gRPC
#include "controllers/ExternalObjectRequest.h"

// std
#include <cstddef>
#include <filesystem>
#include <fstream>

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCExternalObjectRequestTests : public GRPCTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCExternalObjectRequestTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    // Called after each individual test
    void TearDown() override {
        cleanupTestFiles();
        GRPCTest::TearDown();
    }

    // Creates an ExternalObjectRequest handler object.
    void makeOne() override { new ExternalObjectRequest(&service_, dms_, true); }

    // This is a test class which makes an async RPC to the MockServer on construction and compares the streamed-back response.
    using StreamReader = catena::gRPC::test::StreamReader<st2138::ExternalObjectPayload, st2138::ExternalObjectRequestPayload, 
        std::function<void(grpc::ClientContext*, const st2138::ExternalObjectRequestPayload*, grpc::ClientReadReactor<st2138::ExternalObjectPayload>*)>>;

    // Helper function which initializes an ExternalObjectRequestPayload object.
    void initPayload(const std::string& oid) {
        inVal_.set_oid(oid);
    }

    // Creates a test file with specified content for testing file operations.
    void createTestFile(const std::string& filename, const std::string& content) {
        std::string fullPath = testEOPath_ + filename;
        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        std::ofstream file(fullPath, std::ios::binary);
        file << content;
        file.close();
    }

    // Removes test files created during testing.
    void cleanupTestFiles() {
        std::filesystem::remove_all(testEOPath_);
    }

    // Adds an expected external object payload to the expected values.
    void expPayload(const std::string& content) {
        expVals_.push_back(st2138::ExternalObjectPayload());
        expVals_.back().mutable_payload()->set_payload(content.data(), content.size());
    }

    // Makes an async RPC to the MockServer and waits for responses before comparing output.
    void testRPC() {
        // Sending async RPC.
        StreamReader streamReader(&outVals_, &outRc_);
        streamReader.MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->ExternalObjectRequest(ctx, payload, reactor);
        });
        streamReader.Await();
        
        // Comparing the results.
        ASSERT_EQ(outVals_.size(), expVals_.size()) << "Output missing >= 1 ExternalObjectPayload";
        for (size_t i = 0; i < outVals_.size(); i++) {
            EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString());
        }
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        
        // Make sure another ExternalObjectRequest handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // Test file path for external objects
    std::string testEOPath_ = "/tmp/catena_test_eo/";
    
    // Input/output values
    st2138::ExternalObjectRequestPayload inVal_;
    std::vector<st2138::ExternalObjectPayload> outVals_;
    
    // Expected variables
    std::vector<st2138::ExternalObjectPayload> expVals_;

    gRPCExternalObjectRequestTests() : GRPCTest() {
        // Set up mock service EOPath expectations
        EXPECT_CALL(service_, EOPath()).WillRepeatedly(::testing::ReturnRef(testEOPath_));
    }
};

/*
 * ============================================================================
 *                               ExternalObjectRequest tests
 * ============================================================================
 * 
 * ============================================================================
 *                               Basic Functionality Tests
 * ============================================================================
 *
 * TEST 1.1 - Creating an ExternalObjectRequest object.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_Create) {
    EXPECT_TRUE(asyncCall_);
}

/* 
 * TEST 1.2 - Normal case for ExternalObjectRequest proceed() - successful file read.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_Normal) {
    // Create test file with content
    std::string testContent = "This is test file content for external object.";
    createTestFile("/test_file.txt", testContent);
    
    // Initialize request payload
    initPayload("/test_file.txt");
    expPayload(testContent);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 1.3 - ExternalObjectRequest with special characters in filename.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_SpecialCharacters) {
    // Create test file with special characters in name
    std::string specialPath = "/file_with_spaces and-dashes_123.txt";
    std::string testContent = "File with special characters in name.";
    createTestFile(specialPath, testContent);
    
    // Initialize request payload
    initPayload(specialPath);
    expPayload(testContent);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 1.4 - ExternalObjectRequest with empty file.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_EmptyFile) {
    // Create empty test file
    createTestFile("/empty_file.txt", "");
    
    // Initialize request payload
    initPayload("/empty_file.txt");
    expPayload("");
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 1.5 - ExternalObjectRequest with binary file content.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_BinaryFile) {
    // Create test file with binary content
    std::string binaryContent;
    for (int i = 0; i < 256; ++i) {
        binaryContent += static_cast<char>(i);
    }
    createTestFile("/binary_file.bin", binaryContent);
    
    // Initialize request payload
    initPayload("/binary_file.bin");
    expPayload(binaryContent);
    
    // Send the RPC
    testRPC();
}

/* 
 * ============================================================================
 *                               Error Handling Tests
 * ============================================================================
 *
 * TEST 2.1 - ExternalObjectRequest file not found error.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_FileNotFound) {
    // Initialize request payload with non-existent file
    initPayload("/nonexistent_file.txt");
    
    // Set expected error - the actual error message includes function signature
    expRc_ = catena::exception_with_status("virtual void catena::gRPC::ExternalObjectRequest::proceed(bool)\nfile '/nonexistent_file.txt' not found", catena::StatusCode::NOT_FOUND);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 2.2 - ExternalObjectRequest OID validation - missing '/' prefix.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_MissingSlashPrefix) {
    // Initialize request payload with OID missing '/' prefix
    initPayload("test_file_no_slash.txt");
    
    // Set expected error with hint about missing '/' prefix - includes function signature
    expRc_ = catena::exception_with_status("virtual void catena::gRPC::ExternalObjectRequest::proceed(bool)\nfile 'test_file_no_slash.txt' not found. HINT: Make sure oid starts with '/' prefix.", catena::StatusCode::NOT_FOUND);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 2.3 - ExternalObjectRequest with path traversal attempt (security test).
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_PathTraversal) {
    // Initialize request payload with path traversal attempt
    initPayload("/../../../etc/passwd");
    
    // Set expected error - the actual error message includes function signature
    expRc_ = catena::exception_with_status("virtual void catena::gRPC::ExternalObjectRequest::proceed(bool)\nfile '/../../../etc/passwd' not found", catena::StatusCode::NOT_FOUND);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 2.4 - ExternalObjectRequest with file I/O exception (generic exception handling).
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_FileIOException) {
    // Create a file that exists but will cause an I/O exception when read
    // We'll create a directory with the same name as the file to trigger an exception
    std::string problematicPath = "/problematic_file.txt";
    
    // Create a directory instead of a file to cause std::ifstream to fail
    std::filesystem::create_directories(testEOPath_ + problematicPath);
    
    // Initialize request payload
    initPayload(problematicPath);
    
    // Set expected error - generic exception should result in CANCELLED status
    expRc_ = catena::exception_with_status("", catena::StatusCode::CANCELLED);
    
    // Send the RPC
    testRPC();
}
