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
 * @author jason.chen@rossvideo.com
 * @author keon.foster@rossvideo.com
 * @date 2026-03-20
 * @copyright Copyright © 2026 Ross Video Ltd
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
        config::log_dir = UNITTEST_LOG_DIR;
        config::log_file = true;
        config::log_level = "TRACE";
        config::log_size = 10;
        config::log_count = 3;
        config::log_final_rotation = true;
        Logger::init("gRPCExternalObjectRequestTest");
    }

    static void TearDownTestSuite() {
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
    // digest is the SHA-256 digest of content, base64-encoded.
    void expPayload(const std::string& content, const std::string& digest) {
        expVals_.push_back(st2138::ExternalObjectPayload());
        auto* objPayload = expVals_.back().mutable_payload();
        objPayload->set_payload(content.data(), content.size());
        std::string digestBytes = catena::from_base64(digest);
        objPayload->set_digest(digestBytes.data(), digestBytes.size());
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

    /**
     * Makes an async RPC and checks the requestStart/requestReceived values read by the handler.
     */
    void testRPCTimestamps(std::string input, long expected) {
        // Create a new client context for this call to avoid metadata from previous calls
        grpc::ClientContext context;
        context.AddMetadata("request-start", input);

        // Sending async RPC
        StreamReader streamReader(&outVals_, &outRc_);
        streamReader.MakeCall(&context, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->ExternalObjectRequest(ctx, payload, reactor);
        });
        streamReader.Await();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Clear the response for the next call
        outVals_.erase(outVals_.begin(), outVals_.end());

        EXPECT_EQ(requestStart_, expected);
        EXPECT_TRUE(requestReceived_ > DEFAULT_REQUEST_RECEIVED);
    }

    // Test file path for external objects
    std::string testEOPath_ = "/tmp/catena_test_eo/";

    // Precomputed SHA-256 digests (base64) for test payloads.
    std::string digestNormal_ = "IA6PlY/NIwnLzNC76J+Bz2z+AdeqCq8AF+xPZYkmy08=";
    std::string digestSpecialChars_ = "cqGM/92veCrMRfOrvfp6NlpRdGJzHVO4PSL78rE2vTA=";
    std::string digestEmpty_ = "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";
    std::string digestBinary256_ = "QK/y6dLYki5Hr9RkjmlnSXFYeF+9Hahw5xECZr+USIA=";
    
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
    expPayload(testContent, digestNormal_);
    
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
    expPayload(testContent, digestSpecialChars_);
    
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
    expPayload("", digestEmpty_);
    
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
    expPayload(binaryContent, digestBinary256_);
    
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

/*
 * TEST 2.5 - ExternalObjectRequest with null slot, should handle as a normal case.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_NullSlotCase) {
    // Create test file with content
    std::string testContent = "This is test file content for external object.";
    createTestFile("/test_file.txt", testContent);
    
    // Initialize request payload
    initPayload("/test_file.txt");
    inVal_.clear_slot();
    expPayload(testContent, digestNormal_);
    
    // Send the RPC
    testRPC();
}

/*
 * TEST 2.6 - ExternalObjectRequest with invalid slot, should return NOT_FOUND.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_InvalidSlot) {
    // Create test file with content
    std::string testContent = "This is test file content for external object.";
    createTestFile("/test_file.txt", testContent);

    // Initialize request payload
    initPayload("/test_file.txt");
    inVal_.set_slot(9999); 

    // Expect NOT_FOUND error
    expRc_ = catena::exception_with_status("Device not found in slot 9999", catena::StatusCode::NOT_FOUND);

    // Send the RPC
    testRPC();
}

/*
 * TEST 2.7 - ExternalObjectRequest with slot of of range - should return INVALID_ARGUMENT.
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_SlotOutOfRange) {
    dms_[65536] = &dm0_;

    // Create test file with content
    std::string testContent = "This is test file content for external object.";
    createTestFile("/test_file.txt", testContent);

    // Initialize request payload
    initPayload("/test_file.txt");
    inVal_.set_slot(65536); 

    // Expect NOT_FOUND error
    expRc_ = catena::exception_with_status("slot number out of range", catena::StatusCode::INVALID_ARGUMENT);

    // Send the RPC
    testRPC();
}

/*
 * TEST 3 - Test request-start header read correctly
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_RequestStart) {
    testRPCTimestamps("12345123", 12345123);
    testRPCTimestamps("10", 10);
    testRPCTimestamps("12345678912345", 12345678912345);
    testRPCTimestamps("00012345678912345", 12345678912345);
}

/*
 * TEST 4 - Test invalid request-start header value get set to default
 */
TEST_F(gRPCExternalObjectRequestTests, ExternalObjectRequest_InvalidRequestStart) {
    // Test with non-number in value
    testRPCTimestamps("123@123", DEFAULT_REQUEST_START);
    // Test with period
    testRPCTimestamps("123.123", DEFAULT_REQUEST_START);
    // Test with negative value
    testRPCTimestamps("-123123", DEFAULT_REQUEST_START);
    // Test with leading period
    testRPCTimestamps(".123123", DEFAULT_REQUEST_START);
    // Test with empty value
    testRPCTimestamps("", DEFAULT_REQUEST_START);
    // Test with too large of a value
    testRPCTimestamps(std::string(20, '1'), DEFAULT_REQUEST_START);
}
