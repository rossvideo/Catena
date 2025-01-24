#pragma once

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
//

/**
 * @file AuthInterceptor.h
 * @brief Implements the gRPC Interceptor class
 * @author Zuhayr Sarker
 * @author Benjamin Whitten
 * @date 2025-01-20
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <IParam.h>

#include <grpcpp/grpcpp.h>
#include <jwt-cpp/jwt.h>
#include <string>
#include <map>
#include <ServiceImpl.h>


//Simply for testing purposes


class AuthInterceptor : public grpc::experimental::Interceptor {
    public:
        /**
         * @brief Constructor for the Interceptor class
         * @param info - Pointer to the ServerRpcInfo object
         */
        AuthInterceptor(grpc::experimental::ServerRpcInfo* info);

        /**
         * @brief Intercept method for the Interceptor class
         * @param methods - Pointer to the InterceptorBatchMethods object
         */
        void Intercept(grpc::experimental::InterceptorBatchMethods* methods);

    private:
        /**
         * @brief Validates the token and outputs the claims if valid.
         * @param token - The token to validate.
         * @return bool - True if the token is valid, false otherwise.
         */
        void validateToken(const std::string& token, std::string* claims);
        /**
         * @brief Information on the server. Valid claims are appended to its
         * associated ServerContext.
         */
        grpc::experimental::ServerRpcInfo* info_;
        


};
class AuthInterceptorFactory : public grpc::experimental::ServerInterceptorFactoryInterface {
    public:
        /**
         * @brief Create a new Interceptor object
         * @param info - Pointer to the ServerRpcInfo object
         * @return grpc::experimental::Interceptor* - A pointer to the Interceptor object
         */
        grpc::experimental::Interceptor* CreateServerInterceptor(grpc::experimental::ServerRpcInfo* info) override {
            std::cout << "Creating new interceptor" << std::endl;
            return std::make_unique<AuthInterceptor>(info).release(); //Needs to be converted to "raw" pointer? Is this circular or ideal?
        }
};