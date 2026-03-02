#pragma once

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
 * @file Config.h
 * @brief Runtime configuration variables
 * @author keon.foster@rossvideo.com
 * @date 2026-02-24
 * @copyright Copyright (c) 2026 Ross Video
 */

 #include <string>
 #include <cstdint>
 #include <boost/program_options.hpp>
namespace catena{
namespace common{
namespace config{

/**
* Config variable names
*/
const std::string CATENA_SECURE_COMMS_NAME = "secure_comms";
const std::string CATENA_CERTS_NAME = "certs";
const std::string CATENA_STATIC_ROOT_NAME = "static_root";
const std::string CATENA_CERT_FILE_NAME = "cert_file";
const std::string CATENA_KEY_FILE_NAME = "key_file";
const std::string CATENA_CA_FILE_NAME = "ca_file";
const std::string CATENA_LOG_LEVEL_NAME = "log_level";
const std::string CATENA_LOG_CONSOLE_NAME = "log_console";
const std::string CATENA_LOG_FILE_NAME = "log_file";
const std::string CATENA_LOG_FILE_SIZE_NAME = "log_file_size";
const std::string CATENA_LOG_FILE_COUNT_NAME = "log_file_count";
const std::string CATENA_LOG_DIR_NAME = "log_dir";
const std::string CATENA_PORT_NAME = "port";
const std::string CATENA_DEFAULT_MAX_ARRAY_SIZE_NAME = "default_max_array_size";
const std::string CATENA_DEFAULT_TOTAL_ARRAY_SIZE_NAME = "default_total_array_size";
const std::string CATENA_MAX_CONNECTIONS_NAME = "max_connections";
const std::string CATENA_PRIVATE_CA_NAME = "private_ca";
const std::string CATENA_MUTUAL_AUTHC_NAME = "mutual_authc";
const std::string CATENA_AUTHZ_NAME = "authz";
const std::string CATENA_SILENT_NAME = "silent";
const std::string CATENA_HELP_NAME = "help";


/**
 * Default values of config variables
 */
const std::string CATENA_SECURE_COMMS = "off";
const std::string CATENA_CERTS = "${HOME}/test_certs";
const std::string CATENA_CERT_FILE = "server.crt";
const std::string CATENA_KEY_FILE = "server.key";
const std::string CATENA_CA_FILE = "ca.crt";
const std::string CATENA_STATIC_ROOT = "/";
const std::string CATENA_LOG_LEVEL = "info";
const bool CATENA_LOG_CONSOLE = true;
const bool CATENA_LOG_FILE = true;
const uint32_t LOG_FILE_SIZE = 10000000; //10MB
const uint32_t LOG_FILE_COUNT = 5;
const uint16_t CATENA_PORT = 6254;


/**
 * @brief Parses environment variables and command line to initialize variables used for runtime configuration
 * @param argc Command-line argument count
 * @param argv Command-line arguments
 * @param serviceType The type of service being ran, can be "REST" or "GRPC".
 * @param prefix Prefix used for relevant environment variables, include trailing underscore. Defaults to "CATENA_".
 * @return <bool exit, int code>. Exit states if program must be exited, either due to --help or an exception.
 *  Code denotes the reason for exit, with 0 being --help and 1 being an exception. 
 *  The function will print the relevant material to the terminal before returning.
 */
std::pair<bool, int> initConfigVariables(int argc, char* argv[], std::string prefix = "CATENA_");

inline std::string certs = "";

inline std::string secure_comms = "";

inline std::string cert_file = "";

inline std::string key_file = "";

inline std::string ca_file = "";

inline std::string static_root = "";

inline std::string log_level = "";

inline bool log_console = false;

inline bool log_file = false;

inline uint32_t log_file_size = 0;

inline uint32_t log_file_count = 0;

inline std::string log_dir = "";

inline uint32_t default_max_array_size = 0;

inline uint32_t default_total_array_size = 0;

inline uint32_t max_connections = 0;

inline uint16_t port = 0;

inline bool private_ca = false;

inline bool authz = false;

inline bool mutual_authc = false;

inline bool silent = false;

} // namespace config     
} // namespace common
} // namespace catena
