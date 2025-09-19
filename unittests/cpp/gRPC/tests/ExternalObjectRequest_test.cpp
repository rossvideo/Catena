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
        std::filesystem::create_directories(std::filesystem::path(testEOPath_).parent_path());
        std::ofstream file(testEOPath_ + filename);
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
 *                          ExternalObjectRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating an ExternalObjectRequest object.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_Create) {
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for ExternalObjectRequest proceed() - successful file read.
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
    
    // Cleanup
    cleanupTestFiles();
}

// Working on these test right now.

/*
 * TEST 3 - ExternalObjectRequest with empty file.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_EmptyFile) {
    // Create empty test file
    createTestFile("/empty_file.txt", "");
    
    // Initialize request payload
    initPayload("/empty_file.txt");
    expPayload("");
    
    // Send the RPC
    testRPC();
    
    // Cleanup
    cleanupTestFiles();
} */

/*
 * TEST 4 - ExternalObjectRequest file not found error.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_FileNotFound) {
    // Initialize request payload with non-existent file
    initPayload("/nonexistent_file.txt");
    
    // Set expected error
    expRc_ = catena::exception_with_status("file '/nonexistent_file.txt' not found", catena::StatusCode::NOT_FOUND);
    
    // Send the RPC
    testRPC();
} */

/*
 * TEST 5 - ExternalObjectRequest OID validation - missing '/' prefix.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_MissingSlashPrefix) {
    // Initialize request payload with OID missing '/' prefix
    initPayload("test_file_no_slash.txt");
    
    // Set expected error with hint about missing '/' prefix
    expRc_ = catena::exception_with_status("file 'test_file_no_slash.txt' not found. HINT: Make sure oid starts with '/' prefix.", catena::StatusCode::NOT_FOUND);
    
    // Send the RPC
    testRPC();
} */

/*
 * TEST 6 - ExternalObjectRequest with large file content.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_LargeFile) {
    // Create test file with large content (1MB)
    std::string largeContent;
    largeContent.reserve(1024 * 1024);
    for (int i = 0; i < 1024 * 1024; ++i) {
        largeContent += static_cast<char>('A' + (i % 26));
    }
    createTestFile("/large_file.txt", largeContent);
    
    // Initialize request payload
    initPayload("/large_file.txt");
    expPayload(largeContent);
    
    // Send the RPC
    testRPC();
    
    // Cleanup
    cleanupTestFiles();
} */

/*
 * TEST 7 - ExternalObjectRequest with nested directory structure.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_NestedDirectory) {
    // Create nested directory structure
    std::string nestedPath = "/nested/dir/structure/file.txt";
    std::string testContent = "Content in nested directory.";
    
    // Create directories and file
    std::filesystem::create_directories(testEOPath_ + "/nested/dir/structure");
    createTestFile(nestedPath, testContent);
    
    // Initialize request payload
    initPayload(nestedPath);
    expPayload(testContent);
    
    // Send the RPC
    testRPC();
    
    // Cleanup
    cleanupTestFiles();
} */

/*
 * TEST 8 - ExternalObjectRequest with special characters in filename.
 */
/* TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_SpecialCharacters) {
    // Create test file with special characters in name
    std::string specialPath = "/file_with_spaces and-dashes_123.txt";
    std::string testContent = "File with special characters in name.";
    createTestFile(specialPath, testContent);
    
    // Initialize request payload
    initPayload(specialPath);
    expPayload(testContent);
    
    // Send the RPC
    testRPC();
    
    // Cleanup
    cleanupTestFiles();
} */