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
 * @brief This file is for testing the Config.cpp file.
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @author Nathan Rochon (nathan.rochon@rossvideo.com)
 * @date 2026-03-20
 * @copyright Copyright © 2026 Ross Video Ltd
 */

 // gtest
 #include <gtest/gtest.h>

 // common
 #include <Config.h>
 #include <Logger.h>
 #include <Device.h>

 using namespace catena::common;

 // Test fixture for Config tests
 class ConfigTest : public testing::Test {
    protected:
        // Set up and tear down Google Logging
        static void SetUpTestSuite() {
            config::log_dir = UNITTEST_LOG_DIR;
            config::log_file = true;
            config::log_level = "TRACE";
            config::log_size = 10;
            config::log_count = 3;
            config::log_final_rotation = true;
            Logger::init("ConfigTest");
        }

        static void TearDownTestSuite() {
        }

        void SetUp() override {
            // Reset config vars
            config::ca_file = "";
            config::cert_file = "";
            config::certs = "";
            config::key_file = "";
            config::log_dir = "";
            config::secure_comms = "";
            config::static_root = "";
            config::default_max_array_size = 0;
            config::default_total_array_size = 0;
            config::max_connections = 0;
            config::port = 0;
            config::authz = false;
            config::mutual_authc = false;
            config::private_ca = false;
            config::silent = false;
            config::log_level = "";
            config::log_console = false;
            config::log_file = false;
            config::log_size = 0;
            config::log_max_size = 0;
            config::log_count = 0;
            config::log_final_rotation = false;
        }

        void TearDown() override {
            // Reset config vars
            config::ca_file = "";
            config::cert_file = "";
            config::certs = "";
            config::key_file = "";
            config::log_dir = "";
            config::secure_comms = "";
            config::static_root = "";
            config::default_max_array_size = 0;
            config::default_total_array_size = 0;
            config::max_connections = 0;
            config::port = 0;
            config::authz = false;
            config::mutual_authc = false;
            config::private_ca = false;
            config::silent = false;
            config::log_level = "";
            config::log_console = false;
            config::log_file = false;
            config::log_size = 0;
            config::log_max_size = 0;
            config::log_count = 0;
            config::log_final_rotation = false;

            // Reset "HOME" in case missing "HOME" test case couldn't
            if (home != nullptr) {
                setenv("HOME", home, 0);
            }

            // Delete set environment variables
            for (auto var : setVars) {
                unsetenv(var.c_str());
            }
            setVars.clear();
        }

        // Build fake argv from given strings.
        // argv[0] is the program name (required by parse_command_line).
        void buildArgv(std::vector<std::string>& args, int& argc, std::vector<char*>& argv) {
            argc = static_cast<int>(args.size());
            argv.clear();
            argv.reserve(args.size() + 1);
            for (auto& s : args) {
                argv.push_back(s.data());
            }
            argv.push_back(nullptr); //Finish with null
        }

        // Set environment variables from given strings with format "NAME=VALUE"
        void setEnvVars(std::vector<std::string> vars) {
            for (std::string var : vars) {
                std::size_t index = var.find("=");
                std::string name = var.substr(0, index);
                setVars.emplace_back(name);
                std::string val;
                if (index != std::string::npos) {
                    val = var.substr(index + 1);
                } else {
                    val = "true";
                }
                setenv(name.c_str(), val.c_str(), 0);
            }
        }

        // Unset environment variables from given strings. Format "NAME=VALUE" supported.
        void unsetEnvVars(std::vector<std::string> vars) {
            for (std::string var : vars) {
                std::size_t index = var.find("=");
                std::string name = var.substr(0, index);
                unsetenv(name.c_str());
            }
        }

        char* home = nullptr;

        std::vector<std::string> setVars;
    };
    
/**
 * TEST 1 - Unset flags/env vars set to default values
 */
TEST_F(ConfigTest, defaultValues) {
    // Create command line args
    std::vector<std::string> args = {"./test"};
    int argc;
    std::vector<char*> argv;
    buildArgv(args, argc, argv);

    // Set config variables
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "NONE_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    // Check values
    EXPECT_EQ(config::ca_file, config::CA_FILE_DEFAULT);
    EXPECT_EQ(config::cert_file, config::CERT_FILE_DEFAULT);
    EXPECT_EQ(config::certs, config::CERTS_DEFAULT);
    EXPECT_EQ(config::key_file, config::KEY_FILE_DEFAULT);
    EXPECT_EQ(config::log_dir, LOG_DIR);
    EXPECT_EQ(config::secure_comms, config::SECURE_COMMS_DEFAULT);
    EXPECT_EQ(config::static_root, getenv("HOME"));
    EXPECT_EQ(config::default_max_array_size, kDefaultMaxArrayLength);
    EXPECT_EQ(config::default_total_array_size, kDefaultMaxArrayLength);
    EXPECT_EQ(config::max_connections, DEFAULT_MAX_CONNECTIONS);
    EXPECT_EQ(config::port, config::PORT_DEFAULT);
    EXPECT_EQ(config::authz, false);
    EXPECT_EQ(config::mutual_authc, false);
    EXPECT_EQ(config::private_ca, false);
    EXPECT_EQ(config::silent, false);
    EXPECT_EQ(config::log_level, "TRACE");
    EXPECT_EQ(config::log_console, config::LOG_CONSOLE_DEFAULT);
    EXPECT_EQ(config::log_file, config::LOG_FILE_DEFAULT);
    EXPECT_DOUBLE_EQ(config::log_size, config::LOG_SIZE_DEFAULT);
    EXPECT_EQ(config::log_count, config::LOG_COUNT_DEFAULT);
    EXPECT_DOUBLE_EQ(config::log_max_size, config::LOG_MAX_SIZE_DEFAULT);
}

/**
 * TEST 2 - All values set through command-line
 */
TEST_F(ConfigTest, CommandLine) {
    // Create command line args
    std::vector<std::string> args = {
        "./test",
        "--ca_file=a",
        "--cert_file=a",
        "--certs=a",
        "--key_file=a",
        "--log_dir=a",
        "--secure_comms=a",
        "--static_root=a",
        "--default_max_array_size=1",
        "--default_total_array_size=1",
        "--max_connections=1",
        "--port=1",
        "--authz",
        "--mutual_authc",
        "--private_ca",
        "--silent",
        "--log_level=warning",
        "--log_console=false",
        "--log_file=false",
        "--log_size=3",
        "--log_count=7",
        "--log_max_size=100"
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(args, argc, argv);

    // Set config variables
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "NONE_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    // Check values
    EXPECT_EQ(config::ca_file, "a");
    EXPECT_EQ(config::cert_file, "a");
    EXPECT_EQ(config::certs, "a");
    EXPECT_EQ(config::key_file, "a");
    EXPECT_EQ(config::log_dir, "a");
    EXPECT_EQ(config::secure_comms, "a");
    EXPECT_EQ(config::static_root, "a");
    EXPECT_EQ(config::default_max_array_size, 1);
    EXPECT_EQ(config::default_total_array_size, 1);
    EXPECT_EQ(config::max_connections, 1);
    EXPECT_EQ(config::port, 1);
    EXPECT_EQ(config::authz, true);
    EXPECT_EQ(config::mutual_authc, true);
    EXPECT_EQ(config::private_ca, true);
    EXPECT_EQ(config::silent, true);
    EXPECT_EQ(config::log_level, "WARNING");
    EXPECT_EQ(config::log_console, false);
    EXPECT_EQ(config::log_file, false);
    EXPECT_DOUBLE_EQ(config::log_size, 3.0);
    EXPECT_EQ(config::log_count, 7);
    EXPECT_DOUBLE_EQ(config::log_max_size, 100.0);
}

/**
 * TEST 3 - All values set through environment variables
 */
TEST_F(ConfigTest, EnvironmentVariables) {
    // Create environment vairbales
    std::vector<std::string> args = {
        "CONFIGTEST_CA_FILE=a",
        "CONFIGTEST_CERT_FILE=a",
        "CONFIGTEST_CERTS=a",
        "CONFIGTEST_KEY_FILE=a",
        "CONFIGTEST_LOG_DIR=a",
        "CONFIGTEST_SECURE_COMMS=a",
        "CONFIGTEST_STATIC_ROOT=a",
        "CONFIGTEST_DEFAULT_MAX_ARRAY_SIZE=1",
        "CONFIGTEST_DEFAULT_TOTAL_ARRAY_SIZE=1",
        "CONFIGTEST_MAX_CONNECTIONS=1",
        "CONFIGTEST_PORT=1",
        "CONFIGTEST_AUTHZ",
        "CONFIGTEST_MUTUAL_AUTHC",
        "CONFIGTEST_PRIVATE_CA",
        "CONFIGTEST_SILENT",
        "CONFIGTEST_LOG_LEVEL=error",
        "CONFIGTEST_LOG_CONSOLE=false",
        "CONFIGTEST_LOG_FILE=false",
        "CONFIGTEST_LOG_SIZE=4",
        "CONFIGTEST_LOG_COUNT=8"
    };
    setEnvVars(args);

    // Set required command line arg
    char arg0[] = {"./test"};
    char* argv[] = {arg0, nullptr};
    int argc = 1;

    // Set config variables
    const auto [exit, code] = config::initConfigVariables(argc, argv, "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    // Check values
    EXPECT_EQ(config::ca_file, "a");
    EXPECT_EQ(config::cert_file, "a");
    EXPECT_EQ(config::certs, "a");
    EXPECT_EQ(config::key_file, "a");
    EXPECT_EQ(config::log_dir, "a");
    EXPECT_EQ(config::secure_comms, "a");
    EXPECT_EQ(config::static_root, "a");
    EXPECT_EQ(config::default_max_array_size, 1);
    EXPECT_EQ(config::default_total_array_size, 1);
    EXPECT_EQ(config::max_connections, 1);
    EXPECT_EQ(config::port, 1);
    EXPECT_EQ(config::authz, true);
    EXPECT_EQ(config::mutual_authc, true);
    EXPECT_EQ(config::private_ca, true);
    EXPECT_EQ(config::silent, true);
    EXPECT_EQ(config::log_level, "ERROR");
    EXPECT_EQ(config::log_console, false);
    EXPECT_EQ(config::log_file, false);
    EXPECT_DOUBLE_EQ(config::log_size, 4.0);
    EXPECT_EQ(config::log_count, 8);
    EXPECT_DOUBLE_EQ(config::log_max_size, config::LOG_MAX_SIZE_DEFAULT);
}

/**
 * TEST 4 - Values set throug command line and environment variables
 */
TEST_F(ConfigTest, CmdAndEnv) {
    // Create environment vairbales
    std::vector<std::string> envArgs = {
        "CONFIGTEST_CA_FILE=a",
        "CONFIGTEST_DEFAULT_MAX_ARRAY_SIZE=1",
        "CONFIGTEST_AUTHZ",
    };
    setEnvVars(envArgs);

    // Create command line args
        std::vector<std::string> args = {
        "./test",
        "--cert_file=a",
        "--default_total_array_size=1",
        "--mutual_authc"
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(args, argc, argv);

    // Set config variables
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    // Check values set by environment variables
    EXPECT_EQ(config::ca_file, "a");
    EXPECT_EQ(config::default_max_array_size, 1);
    EXPECT_EQ(config::authz, true);
    // Check values set by command line args
    EXPECT_EQ(config::cert_file, "a");
    EXPECT_EQ(config::default_total_array_size, 1);
    EXPECT_EQ(config::mutual_authc, true);
    // Check default values
    EXPECT_EQ(config::certs, config::CERTS_DEFAULT);
    EXPECT_EQ(config::key_file, config::KEY_FILE_DEFAULT);
    EXPECT_EQ(config::log_dir, LOG_DIR);
    EXPECT_EQ(config::secure_comms, config::SECURE_COMMS_DEFAULT);
    EXPECT_EQ(config::static_root, getenv("HOME"));
    EXPECT_EQ(config::max_connections, DEFAULT_MAX_CONNECTIONS);
    EXPECT_EQ(config::port, config::PORT_DEFAULT);
    EXPECT_EQ(config::private_ca, false);
    EXPECT_EQ(config::silent, false);
    EXPECT_EQ(config::log_level, "TRACE");
    EXPECT_EQ(config::log_console, config::LOG_CONSOLE_DEFAULT);
    EXPECT_EQ(config::log_file, config::LOG_FILE_DEFAULT);
    EXPECT_DOUBLE_EQ(config::log_size, config::LOG_SIZE_DEFAULT);
    EXPECT_EQ(config::log_count, config::LOG_COUNT_DEFAULT);
    EXPECT_DOUBLE_EQ(config::log_max_size, config::LOG_MAX_SIZE_DEFAULT);
}

/**
 * TEST 5 - Command line overwrites environment variables
 */
TEST_F(ConfigTest, CmdOverwritesEnv) {
    // Create environment vairbales
    std::vector<std::string> envArgs = {
        "CONFIGTEST_CA_FILE=a",
        "CONFIGTEST_CERT_FILE=a",
        "CONFIGTEST_CERTS=a",
        "CONFIGTEST_KEY_FILE=a",
        "CONFIGTEST_LOG_DIR=a",
        "CONFIGTEST_SECURE_COMMS=a",
        "CONFIGTEST_STATIC_ROOT=a",
        "CONFIGTEST_DEFAULT_MAX_ARRAY_SIZE=1",
        "CONFIGTEST_DEFAULT_TOTAL_ARRAY_SIZE=1",
        "CONFIGTEST_MAX_CONNECTIONS=1",
        "CONFIGTEST_PORT=1",
        "CONFIGTEST_AUTHZ",
        "CONFIGTEST_MUTUAL_AUTHC",
        "CONFIGTEST_PRIVATE_CA",
        "CONFIGTEST_SILENT",
        "CONFIGTEST_LOG_LEVEL=warning",
        "CONFIGTEST_LOG_CONSOLE=true",
        "CONFIGTEST_LOG_FILE=true",
        "CONFIGTEST_LOG_SIZE=2",
        "CONFIGTEST_LOG_COUNT=3",
        "CONFIGTEST_LOG_MAX_SIZE=40"
    };
    setEnvVars(envArgs);

    // Create command line args
    std::vector<std::string> cmdArgs = {
        "./test",
        "--ca_file=b",
        "--cert_file=b",
        "--certs=b",
        "--key_file=b",
        "--log_dir=b",
        "--secure_comms=b",
        "--static_root=b",
        "--default_max_array_size=10",
        "--default_total_array_size=10",
        "--max_connections=10",
        "--port=10",
        "--authz=false",
        "--mutual_authc=false",
        "--private_ca=false",
        "--silent=false",
        "--log_level=error",
        "--log_console=false",
        "--log_file=false",
        "--log_size=9",
        "--log_count=11",
        "--log_max_size=120"
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(cmdArgs, argc, argv);

    // Set config variables
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    // Check values match command line args
    EXPECT_EQ(config::ca_file, "b");
    EXPECT_EQ(config::cert_file, "b");
    EXPECT_EQ(config::certs, "b");
    EXPECT_EQ(config::key_file, "b");
    EXPECT_EQ(config::log_dir, "b");
    EXPECT_EQ(config::secure_comms, "b");
    EXPECT_EQ(config::static_root, "b");
    EXPECT_EQ(config::default_max_array_size, 10);
    EXPECT_EQ(config::default_total_array_size, 10);
    EXPECT_EQ(config::max_connections, 10);
    EXPECT_EQ(config::port, 10);
    EXPECT_EQ(config::authz, false);
    EXPECT_EQ(config::mutual_authc, false);
    EXPECT_EQ(config::private_ca, false);
    EXPECT_EQ(config::silent, false);
    EXPECT_EQ(config::log_level, "ERROR");
    EXPECT_EQ(config::log_console, false);
    EXPECT_EQ(config::log_file, false);
    EXPECT_DOUBLE_EQ(config::log_size, 9.0);
    EXPECT_EQ(config::log_count, 11);
    EXPECT_DOUBLE_EQ(config::log_max_size, 120.0);
}

/**
 * TEST 6 - Missing "HOME" environment variable changes static_root default to "/"
 */
TEST_F(ConfigTest, MissingHome) {

    // Set required command line arg
    char arg0[] = {"./test"};
    char* argv[] = {arg0, nullptr};
    int argc = 1;

    // Save "HOME" value and delete environment variable
    home = getenv("HOME");
    unsetenv("HOME");

    // Do call resulting in new default
    const auto [exit, code] = config::initConfigVariables(argc, argv, "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);
    EXPECT_EQ(config::static_root, "/");

    // Reset "HOME"
    setenv("HOME", home, 0);
}

/**
 * TEST 7 - --help prints message and returns exit with code 0
 */
TEST_F(ConfigTest, HelpMessage) {

    // Set required command line args
    char arg0[] = {"./test"};
    char arg1[] = {"--help"};
    char* argv[] = {arg0, arg1, nullptr};
    int argc = 2;

    // Do call resulting in exit
    const auto [exit, code] = config::initConfigVariables(argc, argv, "CONFIGTEST_");
    EXPECT_TRUE(exit);
    EXPECT_EQ(code, 0);
}

/**
 * TEST 8 - Invalid options dont throw exception
 */
TEST_F(ConfigTest, InvalidOption) {
    // Create command line args
    std::vector<std::string> cmdArgs = {
        "./test",
        "--wrong=b",
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(cmdArgs, argc, argv);
    
    // Expect throw causing exit and code=1
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "CONFIGTEST_");
    EXPECT_TRUE(exit);
    EXPECT_EQ(code, 1);
    
    // Create environment vairbales
    std::vector<std::string> envArgs = {
        "CONFIGTEST_WRONG",
    };
    setEnvVars(envArgs);

    char arg0[] = {"./test"};
    char* argv2[] = {arg0, nullptr};
    argc = 1;

    //Don't expect throw causing exit and code=1
    const auto [exit2, code2] = config::initConfigVariables(argc, argv2, "CONFIGTEST_");
    EXPECT_FALSE(exit2);
    EXPECT_EQ(code2, 0);
}

/**
 * TEST 9 - log_max_size sets log_count and log_size
 */
TEST_F(ConfigTest, LogMaxSize) {
    // Create command line args
    std::vector<std::string> cmdArgs = {
        "./test",
        "--log_max_size=100",
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(cmdArgs, argc, argv);

    // Do call resulting in new default
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    EXPECT_EQ(config::log_max_size, 100);
    EXPECT_EQ(config::log_count, 10);
    EXPECT_EQ(config::log_size, 10);
}

/**
 * TEST 9 - ignored log_max_size doesn't set log_count and log_size
 */
TEST_F(ConfigTest, IgnoredLogMaxSize) {
    // Create command line args
    std::vector<std::string> cmdArgs = {
        "./test",
        "--log_max_size=100",
        "--log_count=20",
        "--log_size=50"
    };
    int argc;
    std::vector<char*> argv;
    buildArgv(cmdArgs, argc, argv);

    // Do call resulting in new default
    const auto [exit, code] = config::initConfigVariables(argc, argv.data(), "CONFIGTEST_");
    EXPECT_FALSE(exit);
    EXPECT_EQ(code, 0);

    EXPECT_EQ(config::log_max_size, 100);
    EXPECT_EQ(config::log_count, 20);
    EXPECT_EQ(config::log_size, 50);
}