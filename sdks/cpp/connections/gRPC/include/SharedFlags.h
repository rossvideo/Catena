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

#include <absl/flags/flag.h>

/**
 *  @brief specify the port for the gRPC service, default is 6254.
 */ 
extern absl::Flag<uint16_t> FLAGS_port;

/**
 *  @brief specify the path to the server's certificate and private key default is ${HOME}/test_certs
 */ 
extern absl::Flag<std::string> FLAGS_certs;

/**
 *  @brief turn secure comms on, options are: off, tls; default is off
 */ 
extern absl::Flag<std::string> FLAGS_secure_comms;

/**
 *  @brief name of the certificate file, default is server.crt
 */ 
extern absl::Flag<std::string> FLAGS_cert_file;

/**
 *  @brief name of private key file, default is server.key
 */ 
extern absl::Flag<std::string> FLAGS_key_file;

/**
 *  @brief name of private CA certificate file, default is ca.crt, ignored if private_ca is false.
 */ 
extern absl::Flag<std::string> FLAGS_ca_file;

/**
 *  @brief specify whether to use a private CA, default is no
 */ 
extern absl::Flag<bool> FLAGS_private_ca;

/**
 *  @brief require mutual TLS, default is no, ignored if secure_comms is off
 */ 
extern absl::Flag<bool> FLAGS_mutual_authc;

/**
 *  @brief require access control, default is off
 */ 
extern absl::Flag<bool> FLAGS_authz;

/**
 *  @brief path to the folder from which static objects can be served, default ${HOME}
 */ 
extern absl::Flag<std::string> FLAGS_static_root;

