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
 * @date 2026-02-19
 * @copyright Copyright (c) 2026 Ross Video
 */

 #include <string>
 #include <cstdint>
namespace catena{
namespace common{
namespace config{

/**
 * @brief Parses environment variables and command line to initialize variables used for runtime configuration
 * @param argc Command-line argument count
 * @param argv Command-line arguments
 * @param prefix Prefix used for relevant environment variables, include trailing underscore. Defaults to "CATENA_".
 */
void initConfigVariables(int argc, char* argv[], std::string prefix = "CATENA_");

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

inline uint16_t port = 0;

inline bool private_ca = false;

inline bool authz = false;

inline bool mutual_authc = false;

inline bool silent = false;

} // namespace config     
} // namespace common
} // namespace catena
