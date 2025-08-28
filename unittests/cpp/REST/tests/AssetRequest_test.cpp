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
 * @brief This file is for testing the MultiSetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/14
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include "interface/param.pb.h"
#include <fstream>
#include <sys/stat.h>

// Test helpers
#include "RESTTest.h"
#include "CommonTestHelpers.h"

// REST
#include "controllers/AssetRequest.h"

using namespace catena::common;
using namespace catena::REST;

// Test class that implements AssetRequest for testing both interface and implementation
class TestAssetRequest : public AssetRequest {
  public:
    TestAssetRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms) 
        : AssetRequest(socket, context, dms) {}

    // Expose protected methods for testing
    using AssetRequest::compress;
    using AssetRequest::deflate_compress;
    using AssetRequest::gzip_compress; 
    using AssetRequest::decompress;
    using AssetRequest::deflate_decompress;
    using AssetRequest::gzip_decompress; 
    using AssetRequest::get_last_write_time;
    using AssetRequest::extractPayload;
};

// Fixture
class RESTAssetRequestTests : public RESTEndpointTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTAssetRequestTests");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    RESTAssetRequestTests() : RESTEndpointTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm0_, getDownloadAssetRequest()).WillRepeatedly(::testing::ReturnRef(downloadAssetRequest_));
        EXPECT_CALL(dm0_, getUploadAssetRequest()).WillRepeatedly(::testing::ReturnRef(uploadAssetRequest_));
        EXPECT_CALL(dm0_, getDeleteAssetRequest()).WillRepeatedly(::testing::ReturnRef(deleteAssetRequest_));
        EXPECT_CALL(context_, EOPath()).WillRepeatedly(::testing::ReturnRef(downloadFolder_));

        // Set up default JWS token for tests
        jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor) + ":w");
    }

    void getAssetRequestTest(st2138::DataPayload::PayloadEncoding compression, const std::string& fqoid, const std::string& payload,
                const std::string& digest, int fileSize, const std::string& authz) {
        //establish expectations
        method_ = Method_GET;
        fqoid_ = fqoid;
        slot_ = 0;
        jwsToken_ = getJwsToken(authz);
        
        std::string compressionString = AssetRequest::payloadEncodingToString(compression);
        ON_CALL(context_, hasField("compression")).WillByDefault(::testing::Return(true));
        ON_CALL(context_, fields("compression")).WillByDefault(::testing::ReturnRef(compressionString));

        // Setting the expected response
        expRc_ = catena::exception_with_status("", catena::StatusCode::OK);

        //Calling proceed and testing the output
        endpoint_->proceed();

        //get response from socket
        std::string response = readTotalResponse();

        //grab everything after the header
        response = response.substr(response.find("\r\n\r\n") + 4);

        //parse into fields using json parser
        //check socketreader/socketwriter for json helper function if this doesnt work
        st2138::ExternalObjectPayload obj;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(response), &obj);
        ASSERT_TRUE(status.ok()) << "Failed to parse JSON response: " << status.ToString();
        
        //ASSERT_TRUE(obj.ParseFromString(response));
        EXPECT_EQ(obj.cachable(), true);
        EXPECT_EQ(obj.payload().payload_encoding(), compression);
        EXPECT_EQ(obj.payload().metadata().at("filename"), fileName_);
        EXPECT_EQ(obj.payload().metadata().at("size"), std::to_string(fileSize));
        EXPECT_EQ(catena::to_base64(obj.payload().digest()), digest);
        EXPECT_EQ(catena::to_base64(obj.payload().payload()), payload);
    }

    void postAssetRequestTest(st2138::DataPayload::PayloadEncoding compression, const std::string& fqoid,
                const std::string& payload, std::string authz) {
        //establish expectations
        method_ = Method_POST;
        fqoid_ = fqoid;
        slot_ = 0;
        authzEnabled_ = true;
        jsonBody_ = catena::from_base64(payload);
        jwsToken_ = getJwsToken(authz);

        std::string compressionString = AssetRequest::payloadEncodingToString(compression);
        ON_CALL(context_, hasField("compression")).WillByDefault(::testing::Return(true));
        ON_CALL(context_, fields("compression")).WillByDefault(::testing::ReturnRef(compressionString));

        // Setting the expected response
        expRc_ = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);

        //Calling proceed and testing the output
        testCall();

        //open the uploaded file
        std::ifstream file(downloadFolder_ + fqoid_, std::ios::binary);
        ASSERT_TRUE(file.is_open()) << "Failed to open uploaded file: " << downloadFolder_ + fqoid_;

        //read the data from the file
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        //check the file content
        EXPECT_EQ(catena::to_base64(fileContent), payloadUncompressed_);
    }

    /*
     * Creates a MultiSetValue handler object.
     */
    ICallData* makeOne() override { return AssetRequest::makeOne(serverSocket_, context_, dms_); }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        EXPECT_EQ(readResponse(), expectedResponse(expRc_));
    }

    const std::string downloadFolder_ = std::string(CATENA_UNITTESTS_DIR) + "/cpp/static";
    vdk::signal<void(const std::string&, const IAuthorizer*)> downloadAssetRequest_;
    vdk::signal<void(const std::string&, const IAuthorizer*)> uploadAssetRequest_;
    vdk::signal<void(const std::string&, const IAuthorizer*)> deleteAssetRequest_;

    std::string fileName_ = "catena_logo.png";
    std::string digestUncompressed_ = "ozr4w8IzziM294/wdLaHVVlUkVe75zQ+WtRot6+oXtk=";
    std::string digestGzip_ = "4vBNeQsVuK9+DIImx5hhHQF3XM6GMqu628H+7VSm9xA=";
    std::string digestDeflate_ = "H+5k8tE4TVqxOdxfW9GVV/KYLZ7FgjVSGyz5OG1pvpU=";
    std::string payloadUncompressed_ = "iVBORw0KGgoAAAANSUhEUgAAAK8AAAAfCAYAAACRWJ0AAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAALiIAAC4iAari3ZIAAAGHaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8P3hwYWNrZXQgYmVnaW49J++7vycgaWQ9J1c1TTBNcENlaGlIenJlU3pOVGN6a2M5ZCc/Pg0KPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyI+PHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj48cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0idXVpZDpmYWY1YmRkNS1iYTNkLTExZGEtYWQzMS1kMzNkNzUxODJmMWIiIHhtbG5zOnRpZmY9Imh0dHA6Ly9ucy5hZG9iZS5jb20vdGlmZi8xLjAvIj48dGlmZjpPcmllbnRhdGlvbj4xPC90aWZmOk9yaWVudGF0aW9uPjwvcmRmOkRlc2NyaXB0aW9uPjwvcmRmOlJERj48L3g6eG1wbWV0YT4NCjw/eHBhY2tldCBlbmQ9J3cnPz4slJgLAAACQklEQVR4Xu3a71HbQBAF8N1rADqAVMCkAtJBKkhCCXSQpAKGCjKMG0g6oIOEDuwKAg3o8QErI7+c/nC+O1vo/T6hlef2dm/HgwRmCwTgEVt8T+bDObAE3aF190X24C0IHBCZCw2vzJaGV2brv9/3mqb54+4XHE8x9vskgCsz+8HxFABuQwjXHG+VejgDsAkhnHO8D4B7M7vkeIqx/jIAHzj2Wu5+z7Ehufoe67Nvbzya2Un3Ri6xBucqqA/nrJ2P5fxC6BPbQ+m67cB52+EtkixWmBXM18W5S+bkXKxk7q7uPmrltEj9lXI/OSfijZTQ5syda2otNV+VTd1Tqr5aSuYdW7vW+fID23e6LsK3OC5l5O712Hq1zpeHV2Q2NLwyWwcbXhTAOeRw+Gxy4BzVh7dpmnVsI/J6x9hHAKe19lV9eN39jGNLwt8m++C1j8RfDpRSdXgBfOtet0+luXTXXpJjrX/3dPbH61cdXsl/oB451KXQ8MpsaXhltjS8Mls8vF/pOrc1B0RSOYDTwq837tz9qr2o9Ypn6EEm5//UxnRz16rXBvIO9SJV3/pV67UKCbl5pfNZJCcrvYdDHGhfzrFepBhav0a97u6h/cHdHcAtf2hfXFgbA/DA8RzaWjjOpnwmFa89dU9vxbbWdxzPYWm9FBEREZFlc1ttrs3shm+ImX06230wWG2mPUUDd/b5/N/rQVutf5r5x53PyIvUHnvzPmhwB0xtJHP/QgENbp/UHiP85r+wSRfsiUOSG35xZKpnU1P2iYEGg3YAAAAASUVORK5CYII=";
    std::string payloadGzip_ = "H4sIAAAAAAACA+sM8HPn5ZLiYmBg4PX0cAkC0uuBWJ6DDUhOjJgLJBkYi4PcnRjWnZN5CeSwpDv6OjIwbOzn/pPICuRzFnhEFjMw6CmBMOOqR3cnATW0Z4ZElET4+lgl5+fqJabkJ6XqVeQWgMxisLGvKEhMzk4tUUhKTc/Ms1V/v3u/ukJmiq16uKmvgW+Bc2pGpkdVUWpwlV9IclV2smWKur0dL5dNhRXQhNzUkkSFitycvGKrClslsMFWQDZIWF/JzqYoJc0qyMUNqgLIs1XKKCkpsNLXLy8v1ys31ssvStc3tLS01Dcw0jcy0gWq0C2uzCtJrNDNK1aGGuCSWpxclFlQkpmfpwDiJybll5bYKpWWZqZYpSWmmSalpJjqJiUap+gaGqYk6iamGBvqphgbp5ibGloYpRkmKUGtL8lMQ9ifVwwNBmCA6INk9A31DEBOBrGt/IsyU4GuAFlpZ2ijjyFmo4/mMKgI0K9AFjxkQMEED93UPGCQlgPDTmfKDG4GBiYnTxfHkIq4t7feB952EGD9cDebwaoh5MASpkuOWh5OnCUTljC1cRn1SHtYLWhu4XvDxcT74iOjtvL+Of8K9llHv/hrt3Dq829l+cebWdK4WR4IRtf4PztsuYHl7sLS767fHuhyyAjM5OZdf2ZaW3ja6/33v89a9vPxO3EZf8Nvv1UatI1/H/wowpDnzHFdJn9qlQXzBk7FOcV7+BuqjXc/qeja+M+IQd7iW9S7uvMbFXf+knu9OT/fZtZNz+9uehvZnmut4H++Zm7/0z9OL4RvO7/c9fZA5e2HupN0wmaInrl+dKf3sycxa1LerXtnl/V0i8ffqUX2luqPeqdceHZmbnaWrmn8VN+7wav2RXk9k43efe3nJ+bb5bv0Dm230Et98u7rjaqt56a3h6eYmewximaXjhM5YPmk4EdO0QOZq+3bM2eV5ih/qjp+w51h+dZvobUVd//2JHsfuq/2+8HW/iPxfFOCSisYa17HbfdfurPketyk5LX130u+HX8dLht3sn6BnOWVpRc+nMrJrEjdBjLdZk/8S6vtpuwuQZYNlw+tN3935++qvSu+dqW/cFR7/v/KsZLi66uus33i+6oUev9X6OutXJw7K5d+jlRR197b7iyTEX9uY9wSiaj9bnvfvVv49+Bd2QO69RnxMRHSzB8OfJS5FWVxblmNmuh621L//KPbrpXLnE/M3CsiKCg4MbU4Onfd2Tdt+R0za61u1xpE5C6zdWUu//bzz/3XjtFr62f9PD73/InuK3LVn+0yCth9clWK/zsx8ubNvyKn/OfZ/g2e4m86nSdJ1xWmrEoPDv7W2cjWXAbKxp6ufi7rnBKaADkN4oRABAAA";
    std::string payloadDeflate_ = "eNrrDPBz5+WS4mJgYOD19HAJAtLrgViegw1IToyYCyQZGIuD3J0Y1p2TeQnksKQ7+joyMGzs5/6TyArkcxZ4RBYzMOgpgTDjqkd3JwE1tGeGRJRE+PpYJefn6iWm5Cel6lXkFoDMYrCxryhITM5OLVFISk3PzLNVf797v7pCZoqteripr4FvgXNqRqZHVVFqcJVfSHJVdrJlirq9HS+XTYUV0ITc1JJEhYrcnLxiqwpbJbDBVkA2SFhfyc6mKCXNKsjFDaoCyLNVyigpKbDS1y8vL9crN9bLL0rXN7S0tNQ3MNI3MtIFqtAtrswrSazQzStWhhrgklqcXJRZUJKZn6cA4icm5ZeW2CqVlmamWKUlppkmpaSY6iYlGqfoGhqmJOomphgb6qYYG6eYmxpaGKUZJilBrS/JTEPYn1cMDQZggOiDZPQN9QxATgaxrfyLMlOBrgBZaWdoo48hZqOP5jCoCNCvQBY8ZEDBBA/d1DxgkJYDw05nygxuBgYmJ08Xx5CKuLe33gfedhBg/XA3m8GqIeTAEqZLjloeTpwlE5YwtXEZ9Uh7WC1obuF7w8XE++Ijo7by/jn/CvZZR7/4a7dw6vNvZfnHm1nSuFkeCEbX+D87bLmB5e7C0u+u3x7ocsgIzOTmXX9mWlt42uv997/PWvbz8TtxGX/Db79VGrSNfx/8KMKQ58xxXSZ/apUF8wZOxTnFe/gbqo13P6no2vjPiEHe4lvUu7rzGxV3/pJ7vTk/32bWTc/vbnob2Z5rreB/vmZu/9M/Ti+Ebzu/3PX2QOXth7qTdMJmiJ65fnSn97MnMWtS3q17Z5f1dIvH36lF9pbqj3qnXHh2Zm52lq5p/FTfu8Gr9kV5PZON3n3t5yfm2+W79A5tt9BLffLu642qreemt4enmJnsMYpml44TOWD5pOBHTtEDmavt2zNnleYof6o6fsOdYfnWb6G1FXf/9iR7H7qv9vvB1v4j8XxTgkorGGtex233X7qz5HrcpOS19d9Lvh1/HS4bd7J+gZzllaUXPpzKyaxI3QYy3WZP/Eur7absLkGWDZcPrTd/d+fvqr0rvnalv3BUe/7/yrGS4uurrrN94vuqFHr/V+jrrVycOyuXfo5UUdfe2+4skxF/bmPcEomo/W57371b+PfgXdkDuvUZ8TER0swfDnyUuRVlcW5ZjZroettS//yj266Vy5xPzNwrIigoODG1ODp33dk3bfkdM2utbtcaROQus3VlLv/288/9147Ra+tn/Tw+9/yJ7ity1Z/tMgrYfXJViv87MfLmzb8ip/zn2f4NnuJvOp0nSdcVpqxKDw7+1tnI1lwGysaern4u65wSmgAWfMh2";
};

/*
 * ============================================================================
 *                               AssetRequest tests
 * ============================================================================
 * 
 * TEST 1.1 - GET asset request for a file that does not exist.
 */
TEST_F(RESTAssetRequestTests, GETAssetRequest_DNE) {
    //establish expectations
    method_ = Method_GET;
    fqoid_ = "/test_asset";
    slot_ = 0;
    authzEnabled_ = false;

    // Setting the expected response
    expRc_ = catena::exception_with_status("AssetRequest[0] for file: /test_asset not found", catena::StatusCode::NOT_FOUND);

    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.2 - GET asset request for a file that exists without authorization.
 */
TEST_F(RESTAssetRequestTests, GETAssetRequest_NoAuthz) {
    //establish expectations
    method_ = Method_GET;
    fqoid_ = "/" + fileName_;
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken("");

    // Setting the expected response
    expRc_ = catena::exception_with_status("Not authorized to download asset", catena::StatusCode::PERMISSION_DENIED);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 1.3 - GET asset request for a file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, GETAssetRequest_Exists) {
    getAssetRequestTest(st2138::DataPayload::UNCOMPRESSED, "/" + fileName_, payloadUncompressed_,
            digestUncompressed_, 1088, Scopes().getForwardMap().at(Scopes_e::kMonitor));
}

/*
 * TEST 1.4 - GET asset request for a Gzip encoded file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, GETAssetRequest_ExistsGzip) {
    getAssetRequestTest(st2138::DataPayload::GZIP, "/" + fileName_, payloadGzip_,
            digestGzip_, 1026, Scopes().getForwardMap().at(Scopes_e::kMonitor));
}

/*
 * TEST 1.5 - GET asset request for a Deflate encoded file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, GETAssetRequest_ExistsDeflate) {
    getAssetRequestTest(st2138::DataPayload::DEFLATE, "/" + fileName_, payloadDeflate_,
            digestDeflate_, 1014, Scopes().getForwardMap().at(Scopes_e::kMonitor));
}

/* 
 * TEST 2.1 - POST asset request for a file without authorization.
 */
TEST_F(RESTAssetRequestTests, POSTAssetRequest_NoAuthz) {
    //establish expectations
    method_ = Method_POST;
    fqoid_ = "/test_asset.png";
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));

    // Setting the expected response
    expRc_ = catena::exception_with_status("Not authorized to POST asset", catena::StatusCode::PERMISSION_DENIED);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 2.2 - POST asset request for a file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, POSTAssetRequest_Exists) {
    //establish expectations
    method_ = Method_POST;
    fqoid_ = "/" + fileName_;
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");

    // Setting the expected response
    expRc_ = catena::exception_with_status("file: " + fqoid_ + " already exists", catena::StatusCode::ALREADY_EXISTS);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 2.3 - POST asset request for a file that does not exist with authorization.
 */
TEST_F(RESTAssetRequestTests, POSTAssetRequest_DNE) {
    postAssetRequestTest(st2138::DataPayload::UNCOMPRESSED, "/catena_logo_up.png", payloadUncompressed_,
                Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");
    ASSERT_TRUE(std::filesystem::remove(downloadFolder_ + fqoid_));
}

/*
 * TEST 2.4 - POST asset request for a Gzip encoded file that does not exist with authorization.
 */
TEST_F(RESTAssetRequestTests, POSTAssetRequest_DNE_Gzip) {
    postAssetRequestTest(st2138::DataPayload::GZIP, "/catena_logo_up.png", payloadGzip_,
                Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");
    ASSERT_TRUE(std::filesystem::remove(downloadFolder_ + fqoid_));
}

/*
 * TEST 2.5 - POST asset request for a Deflate encoded file that does not exist with authorization.
 */
TEST_F(RESTAssetRequestTests, POSTAssetRequest_DNE_Deflate) {
    postAssetRequestTest(st2138::DataPayload::DEFLATE, "/catena_logo_up.png", payloadDeflate_,
                Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");
    ASSERT_TRUE(std::filesystem::remove(downloadFolder_ + fqoid_));
}

/*
 * TEST 3.1 - PUT asset request for a file that does not exist.
 */
TEST_F(RESTAssetRequestTests, PUTAssetRequest_DNE) {
    //establish expectations
    method_ = Method_PUT;
    fqoid_ = "/test_asset.jpg";
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");

    // Setting the expected response
    expRc_ = catena::exception_with_status("file: " + fqoid_ + " not found", catena::StatusCode::NOT_FOUND);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 3.2 - PUT asset request for a file that exists without authorization.
 */
TEST_F(RESTAssetRequestTests, PUTAssetRequest_NoAuthz) {
    //establish expectations
    method_ = Method_PUT;
    fqoid_ = "/" + fileName_;
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));

    // Setting the expected response
    expRc_ = catena::exception_with_status("Not authorized to POST asset", catena::StatusCode::PERMISSION_DENIED);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 3.3 - PUT asset request for a file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, PUTAssetRequest_Exists) {
    //establish expectations
    method_ = Method_PUT;
    fqoid_ = "/catena_logo_up.png";
    slot_ = 0;
    authzEnabled_ = true;
    jsonBody_ = catena::from_base64(payloadUncompressed_);
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");

    std::string compressionString = AssetRequest::payloadEncodingToString(st2138::DataPayload::UNCOMPRESSED);
    ON_CALL(context_, hasField("compression")).WillByDefault(::testing::Return(true));
    ON_CALL(context_, fields("compression")).WillByDefault(::testing::ReturnRef(compressionString));

    // Setting the expected response
    expRc_ = catena::exception_with_status("file: " + fqoid_ + " exists, overwriting with PUT", catena::StatusCode::NO_CONTENT);

    // Create the file to overwrite
    std::ofstream fileOut(downloadFolder_ + fqoid_, std::ios::binary);
    ASSERT_TRUE(fileOut.is_open()) << "Failed to create file: " << downloadFolder_ + fqoid_;
    fileOut << "This is a test file to be overwritten.";
    fileOut.close();

    // Calling proceed and testing the output
    testCall();

    //open the file
    std::ifstream fileIn(downloadFolder_ + fqoid_, std::ios::binary);
    ASSERT_TRUE(fileIn.is_open()) << "Failed to open uploaded file: " << downloadFolder_ + fqoid_;

    //read the data from the file
    std::string fileContent((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
    fileIn.close();
    
    //check the file content
    EXPECT_EQ(catena::to_base64(fileContent), payloadUncompressed_);

    // Remove the file after test
    ASSERT_TRUE(std::filesystem::remove(downloadFolder_ + fqoid_));
}

/*
 * TEST 4.1 - DELETE asset request for a file that does not exist.
 */
TEST_F(RESTAssetRequestTests, DELETEAssetRequest_DNE) {
    //establish expectations
    method_ = Method_DELETE;
    fqoid_ = "/test_asset.jpg";
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");

    // Setting the expected response
    expRc_ = catena::exception_with_status("file: " + fqoid_ + " not found", catena::StatusCode::NOT_FOUND);

    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 4.2 - DELETE asset request for a file that exists without authorization.
 */
TEST_F(RESTAssetRequestTests, DELETEAssetRequest_NoAuthz) {
    //establish expectations
    method_ = Method_DELETE;
    fqoid_ = "/test_asset.jpg";
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));

    // Setting the expected response
    expRc_ = catena::exception_with_status("Not authorized to DELETE asset", catena::StatusCode::PERMISSION_DENIED);

    //Create the file to delete
    std::ofstream file(downloadFolder_ + fqoid_, std::ios::binary);
    ASSERT_TRUE(file.is_open()) << "Failed to create file: " << downloadFolder_ + fqoid_;
    file << "This is a test file to be deleted.";
    file.close();

    // Calling proceed and testing the output
    testCall();

    // Remove file and check if it still exists
    ASSERT_TRUE(std::filesystem::exists(downloadFolder_ + fqoid_)) << "File was not deleted: " << downloadFolder_ + fqoid_;
    std::filesystem::remove(downloadFolder_ + fqoid_);
    ASSERT_TRUE(!std::filesystem::exists(downloadFolder_ + fqoid_)) << "File was deleted without authorization: " << downloadFolder_ + fqoid_;
}

/*
 * TEST 4.3 - DELETE asset request for a file that exists with authorization.
 */
TEST_F(RESTAssetRequestTests, DELETEAssetRequest_Exists) {
    //establish expectations
    method_ = Method_DELETE;
    fqoid_ = "/test_asset.jpg";
    slot_ = 0;
    authzEnabled_ = true;
    jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate) + ":w");

    // Setting the expected response
    expRc_ = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);

    //Create the file to delete
    std::ofstream file(downloadFolder_ + fqoid_, std::ios::binary);
    ASSERT_TRUE(file.is_open()) << "Failed to create file: " << downloadFolder_ + fqoid_;
    file << "This is a test file to be deleted.";
    file.close();

    // Calling proceed and testing the output
    testCall();
    ASSERT_TRUE(!std::filesystem::exists(downloadFolder_ + fqoid_)) << "File was not deleted: " << downloadFolder_ + fqoid_;
}

/*
 * TEST 5.1 - Compress failed
 */
TEST_F(RESTAssetRequestTests, CompressFailed) {
    //establish expectations
    std::vector<uint8_t> data = {0x0, 0x1, 0x2, 0x3, 0x4}; // Example data that cannot be compressed
    
    // Execute the compress function and expect an exception
    try {
        TestAssetRequest::compress(data, 100);
    } catch (const catena::exception_with_status& e) {
        EXPECT_EQ(e.status, catena::StatusCode::INTERNAL);
    }
}

/*
 * TEST 5.2 - Deflate compress succeed
 */
TEST_F(RESTAssetRequestTests, CompressSucceed) {
    //establish expectations
    std::vector<uint8_t> data = {0x0, 0x1, 0x2, 0x3, 0x4}; // Example data that cannot be compressed
    std::vector<uint8_t> expectedData = {120, 218, 99, 96, 100, 98, 102, 1, 0, 0, 25, 0, 11}; // Example deflated data
    
    TestAssetRequest::deflate_compress(data);

    // Check if the data is compressed correctly
    EXPECT_EQ(data, expectedData);
}

/*
 * TEST 5.3 - Decompress failed
 */
TEST_F(RESTAssetRequestTests, DecompressFailed) {
    //establish expectations
    std::vector<uint8_t> data = {0x0, 0x1, 0x2, 0x3, 0x4}; // Example data that cannot be compressed
    try {
        TestAssetRequest::decompress(data, 100);
    } catch (const catena::exception_with_status& e) {
        EXPECT_EQ(e.status, catena::StatusCode::INTERNAL);
    }
}

/*
 * TEST 5.4 - Decompress succeed
 */
TEST_F(RESTAssetRequestTests, DecompressSucceed) {
    //establish expectations
    std::vector<uint8_t> data = {120, 218, 99, 96, 100, 98, 102, 1, 0, 0, 25, 0, 11}; // Example deflated data
    std::vector<uint8_t> expectedData = {0x0, 0x1, 0x2, 0x3, 0x4}; // Example original data
    
    TestAssetRequest::deflate_decompress(data);

    // Check if the data is decompressed correctly
    EXPECT_EQ(data, expectedData);
}

//extract empty payload
TEST_F(RESTAssetRequestTests, ExtractPayloadDNE) {
    //establish expectations
    fqoid_ = "/empty_file";

    std::string compressionString = AssetRequest::payloadEncodingToString(st2138::DataPayload::UNCOMPRESSED);
    ON_CALL(context_, hasField("compression")).WillByDefault(::testing::Return(true));
    ON_CALL(context_, fields("compression")).WillByDefault(::testing::ReturnRef(compressionString));

    //extract the payload from non existant file
    try {
        static_cast<TestAssetRequest*>(endpoint_.release())->extractPayload(downloadFolder_ + fqoid_);
    } catch (const catena::exception_with_status& e) {
        EXPECT_EQ(e.status, catena::StatusCode::NOT_FOUND);
    }
}