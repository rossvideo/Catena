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
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-03-20
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
const std::string SECURE_COMMS_KEY = "secure_comms";
const std::string CERTS_KEY = "certs";
const std::string STATIC_ROOT_KEY = "static_root";
const std::string CERT_FILE_KEY = "cert_file";
const std::string KEY_FILE_KEY = "key_file";
const std::string CA_FILE_KEY = "ca_file";
const std::string LOG_DIR_KEY = "log_dir";
const std::string HOSTNAME_KEY = "hostname";
const std::string PORT_KEY = "port";
const std::string DASHBOARD_PORT_KEY = "dashboard_port";
const std::string DASHBOARD_TLS_ENABLED_KEY = "dashboard_tls_enabled";
const std::string DEFAULT_MAX_ARRAY_SIZE_KEY = "default_max_array_size";
const std::string DEFAULT_TOTAL_ARRAY_SIZE_KEY = "default_total_array_size";
const std::string MAX_CONNECTIONS_KEY = "max_connections";
const std::string PRIVATE_CA_KEY = "private_ca";
const std::string MUTUAL_AUTHC_KEY = "mutual_authc";
const std::string AUTHZ_KEY = "authz";
const std::string SILENT_KEY = "silent";
const std::string HELP_KEY = "help";
const std::string LOG_LEVEL_KEY = "log_level";
const std::string LOG_CONSOLE_KEY = "log_console";
const std::string LOG_FILE_KEY = "log_file";
const std::string LOG_SIZE_KEY = "log_size";
const std::string LOG_COUNT_KEY = "log_count";
const std::string LOG_MAX_SIZE_KEY = "log_max_size";
const std::string LOG_FINAL_ROTATION_KEY = "log_final_rotation";
const std::string LOG_APPEND_KEY = "log_append";


/**
 * Default values of config variables
 */
const std::string SECURE_COMMS_DEFAULT = "off";
const std::string CERTS_DEFAULT = "${HOME}/test_certs";
const std::string CERT_FILE_DEFAULT = "server.crt";
const std::string KEY_FILE_DEFAULT = "server.key";
const std::string CA_FILE_DEFAULT = "ca.crt";
const std::string STATIC_ROOT_DEFAULT = "/";
const std::string HOSTNAME_DEFAULT = "0.0.0.0";
const uint16_t PORT_DEFAULT = 6254;
const uint16_t DASHBOARD_PORT_DEFAULT = 8080;
const bool DASHBOARD_TLS_ENABLED_DEFAULT = false;
const bool LOG_CONSOLE_DEFAULT = true;
const bool LOG_FILE_DEFAULT = true;
const double LOG_SIZE_DEFAULT = 10.0;
const int LOG_COUNT_DEFAULT = 5;
const double LOG_MAX_SIZE_DEFAULT = 50.0;
const bool LOG_FINAL_ROTATION_DEFAULT = false;
const bool LOG_APPEND_DEFAULT = true;
#ifdef NDEBUG
const std::string LOG_LEVEL_DEFAULT = "debug";
#else
const std::string LOG_LEVEL_DEFAULT = "trace";
#endif


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

inline std::string log_dir = "";

inline uint32_t default_max_array_size = 0;

inline uint32_t default_total_array_size = 0;

inline uint32_t max_connections = 0;

inline std::string hostname = "";

inline uint16_t port = 0;

inline uint16_t dashboard_port = 0;

inline bool dashboard_tls_enabled = false;

inline bool private_ca = false;

inline bool authz = false;

inline bool mutual_authc = false;

inline bool silent = false;

inline std::string log_level = "";

inline bool log_console = false;

inline bool log_file = false;

inline double log_size = 0;

inline double log_max_size = 0;

inline int log_count = 0;

inline bool log_final_rotation = false;

inline bool log_append = false;

} // namespace config     
} // namespace common
} // namespace catena
