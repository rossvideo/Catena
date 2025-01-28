/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.
*/

// common
#include <Tags.h>

// connections/gRPC
#include <AuthInterceptor.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;


#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

const std::string PUBLIC_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu1SU1LfVLPHCozMxH2Mo
4lgOEePzNm0tRgeLezV6ffAt0gunVTLw7onLRnrq0/IzW7yWR7QkrmBL7jTKEn5u
+qKhbwKfBstIs+bMY2Zkp18gnTxKLxoS2tFczGkPLPgizskuemMghRniWaoLcyeh
kd3qqGElvW/VDL5AaWTg0nLVkjRo9z+40RQzuVaE8AkAFmxZzow3x+VJYKdjykkJ
0iT9wCS0DRTXu269V264Vf/3jvredZiKRkgwlL9xNAwxXFg0x/XFw005UWVRIkdg
cKWTjpBP2dPwVZ4WWC+9aGVd+Gyn1o0CLelf4rEjGoXbAAEgAqeGUxrcIlbjXfbc
mwIDAQAB
-----END PUBLIC KEY-----
)"; //Working with public keys for now
const std::string PRIVATE_KEY = R"(
-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQC7VJTUt9Us8cKj
MzEfYyjiWA4R4/M2bS1GB4t7NXp98C3SC6dVMvDuictGeurT8jNbvJZHtCSuYEvu
NMoSfm76oqFvAp8Gy0iz5sxjZmSnXyCdPEovGhLa0VzMaQ8s+CLOyS56YyCFGeJZ
qgtzJ6GR3eqoYSW9b9UMvkBpZODSctWSNGj3P7jRFDO5VoTwCQAWbFnOjDfH5Ulg
p2PKSQnSJP3AJLQNFNe7br1XbrhV//eO+t51mIpGSDCUv3E0DDFcWDTH9cXDTTlR
ZVEiR2BwpZOOkE/Z0/BVnhZYL71oZV34bKfWjQIt6V/isSMahdsAASACp4ZTGtwi
VuNd9tybAgMBAAECggEBAKTmjaS6tkK8BlPXClTQ2vpz/N6uxDeS35mXpqasqskV
laAidgg/sWqpjXDbXr93otIMLlWsM+X0CqMDgSXKejLS2jx4GDjI1ZTXg++0AMJ8
sJ74pWzVDOfmCEQ/7wXs3+cbnXhKriO8Z036q92Qc1+N87SI38nkGa0ABH9CN83H
mQqt4fB7UdHzuIRe/me2PGhIq5ZBzj6h3BpoPGzEP+x3l9YmK8t/1cN0pqI+dQwY
dgfGjackLu/2qH80MCF7IyQaseZUOJyKrCLtSD/Iixv/hzDEUPfOCjFDgTpzf3cw
ta8+oE4wHCo1iI1/4TlPkwmXx4qSXtmw4aQPz7IDQvECgYEA8KNThCO2gsC2I9PQ
DM/8Cw0O983WCDY+oi+7JPiNAJwv5DYBqEZB1QYdj06YD16XlC/HAZMsMku1na2T
N0driwenQQWzoev3g2S7gRDoS/FCJSI3jJ+kjgtaA7Qmzlgk1TxODN+G1H91HW7t
0l7VnL27IWyYo2qRRK3jzxqUiPUCgYEAx0oQs2reBQGMVZnApD1jeq7n4MvNLcPv
t8b/eU9iUv6Y4Mj0Suo/AU8lYZXm8ubbqAlwz2VSVunD2tOplHyMUrtCtObAfVDU
AhCndKaA9gApgfb3xw1IKbuQ1u4IF1FJl3VtumfQn//LiH1B3rXhcdyo3/vIttEk
48RakUKClU8CgYEAzV7W3COOlDDcQd935DdtKBFRAPRPAlspQUnzMi5eSHMD/ISL
DY5IiQHbIH83D4bvXq0X7qQoSBSNP7Dvv3HYuqMhf0DaegrlBuJllFVVq9qPVRnK
xt1Il2HgxOBvbhOT+9in1BzA+YJ99UzC85O0Qz06A+CmtHEy4aZ2kj5hHjECgYEA
mNS4+A8Fkss8Js1RieK2LniBxMgmYml3pfVLKGnzmng7H2+cwPLhPIzIuwytXywh
2bzbsYEfYx3EoEVgMEpPhoarQnYPukrJO4gwE2o5Te6T5mJSZGlQJQj9q4ZB2Dfz
et6INsK0oG8XVGXSpQvQh3RUYekCZQkBBFcpqWpbIEsCgYAnM3DQf3FJoSnXaMhr
VBIovic5l0xFkEHskAjFTevO86Fsz1C2aSeRKSqGFoOQ0tmJzBEs1R6KqnHInicD
TQrKhArgLXX4v3CddjfTRJkFWDbE/CkvKZNOrcf1nhaGCPspRJj2KUkj1Fhl9Cnc
dn/RsYEONbwQSjIfMPkvxF+8HQ==
-----END PRIVATE KEY-----
)";


AuthInterceptor::AuthInterceptor(grpc::experimental::ServerRpcInfo* info) : info_(info) {};

void AuthInterceptor::validateToken(const std::string& token, std::string* claims) {
    /**
     * STEPS:
     *
     * 1. Decode token to verify structure
     * 2. Verify token signature using appropriate key (e.g. public key for RS256)
     * 3. Check claims like expiration time, issuer, audience, etc.
     * 4. Handle invalid tokens by throwing an exception such as UNAUTHENTICATED, return false
     * 5. If token is valid, return true and attach claims and data to context
     * 
     */

    // Decodes the token into its claims. decoded_jwt< traits::open_source_parsers_jsoncpp >
    auto decodedToken = jwt::decode(token);
    // Currently set to verify iss, aud, nbf and exp
    auto verifier = jwt::verify()
        // pubkey, privkey, pubpass, privpass - pending to change based on given JWT
        .allow_algorithm(jwt::algorithm::rs256(PUBLIC_KEY, "", "", ""));
        //.with_issuer("Some given issuer")
        //.with_audience("Placeholder audience");
        //.leeway(60); //Not sure if this is required
        
    // Throws error if the token is invalid. Caught by try-catch in Intercept()
    verifier.verify(decodedToken);

    auto now = std::chrono::system_clock::now();
    std::time_t nowSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Check for token's NBF claim
    if (decodedToken.has_payload_claim("nbf")) {
        std::time_t nbf = decodedToken.get_payload_claim("nbf").as_date().time_since_epoch().count();
        // Ensure nbf is limited to 10 digits
        nbf = static_cast<std::time_t>(nbf / 1000000000);
        if (nowSinceEpoch < nbf) {
            throw std::runtime_error("Token not valid yet (nbf claim)");
        }
        std::cout << "Token is valid with NBF check" << std::endl;
    }

    // Check if token is expired
    if (decodedToken.has_payload_claim("exp")) {
        auto exp = decodedToken.get_payload_claim("exp").as_date().time_since_epoch().count();
        // Ensure exp is limited to 10 digits
        exp = static_cast<std::time_t>(exp / 1000000000);
        if (nowSinceEpoch > exp) {
            throw std::runtime_error("Token expired");
        }
        std::cout << "Token is valid with EXP check" << std::endl;
    }

    // Extracting claims from the token and assigning it to the claims output.
    *claims = decodedToken.get_payload();

    // Is there a point in this???
    //return true;
}

void AuthInterceptor::Intercept(grpc::experimental::InterceptorBatchMethods* methods) {
    bool hijack = false;
    /**
     * If there's a hook point the gRPC call is intercepted before the initial
     * metadata is sent. Authorization data is then found from that.
     */
    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
        status = grpc::Status(grpc::StatusCode::OK, "");

        auto* metadata = methods->GetRecvInitialMetadata(); 

        if (metadata == nullptr) {
            std::cout << "No metadata found" << std::endl;
            status = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "No metadata found");
            return;
        }

        auto authData = metadata->find("authorization");
        // Verfying authData exists and is using Bearer schema.
        if (authData == metadata->end() || !authData->second.starts_with("Bearer ")) {
            std::cout << "Invalid token due to missing or invalid authorization header" << std::endl;
            status = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid Token");
            return;
        }
        // Getting token (after "bearer") and converting to std::string.
        auto tokenSubStr = authData->second.substr(std::string("Bearer ").length());
        std::string JWSToken = std::string(tokenSubStr.begin(), tokenSubStr.end());

        try {
            // Validating token and extracting claims.
            std::string claims;
            validateToken(JWSToken, &claims);
            // Sanitizing metadata.
            metadata->erase("claims");
            if (metadata->find("claims") != metadata->end()) {
                throw std::runtime_error("Claims already exist in metadata");
            }
            /**
             * For some reason you need to save both the string and sting_ref
             * in order for claims to not go out of scope.
             */
            sharedClaims = std::make_shared<std::string>(claims);
            sharedClaimsRef = std::make_shared<grpc::string_ref>(*sharedClaims);
            metadata->insert(std::make_pair("claims", *sharedClaimsRef));
        } catch (const std::exception& e) { // Currently enters THIS catch block.
            std::cout << "Invalid token due to alternate exception: " << e.what() << std::endl;
            status = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, e.what());
            info_->server_context()->TryCancel();
        }

        // Updating status and proceeding with gRPC.
        //grpc::Status status(grpc::StatusCode::OK, "Valid token");
        //methods->ModifySendStatus(status);

    } else {
        std::cout << "No interception hook point found" << std::endl;
    }
    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
        methods->ModifySendStatus(status);
    }
    methods->Proceed();

}
