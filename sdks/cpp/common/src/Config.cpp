#include <boost/program_options.hpp>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <cctype>
#include <iostream>

// Catena
#include <Config.h>
#include <Device.h>
#include <Logger.h>

namespace po = boost::program_options;
using namespace catena::common;

std::unordered_set<std::string> log_levels = {
    "trace",
    "debug",
    "info",
    "warning",
    "error",
    "fatal"
};

std::pair<bool, int> config::initConfigVariables(int argc, char* argv[], std::string prefix) {
    try {
        //Declare options
        const char* home = (getenv("HOME") != nullptr) ? getenv("HOME") : STATIC_ROOT_DEFAULT.c_str(); // TODO: Add windows functionality
        po::options_description configArgs("Allowed options");
        configArgs.add_options()
            (HELP_KEY.c_str(), "Get help message")
            (SECURE_COMMS_KEY.c_str(), po::value<std::string>()->default_value(SECURE_COMMS_DEFAULT), "Specify type of secure comms, options are: \"off\", \"tls\"")
            (CERTS_KEY.c_str(), po::value<std::string>()->default_value(CERTS_DEFAULT), "path/to/certs/files")
            (CERT_FILE_KEY.c_str(), po::value<std::string>()->default_value(CERT_FILE_DEFAULT), "Specify the certificate file")
            (KEY_FILE_KEY.c_str(), po::value<std::string>()->default_value(KEY_FILE_DEFAULT), "Specify the key file")
            (CA_FILE_KEY.c_str(), po::value<std::string>()->default_value(CA_FILE_DEFAULT), "Specify the CA file if using a private CA")
            (STATIC_ROOT_KEY.c_str(), po::value<std::string>()->default_value(home), "Specify the directory to search for external objects")
            (LOG_DIR_KEY.c_str(), po::value<std::string>()->default_value(LOG_DIR), "Specify the directory for log files.")
            (HOSTNAME_KEY.c_str(), po::value<std::string>()->default_value(HOSTNAME_DEFAULT), "Specify the publicly accessible hostname")
            (PORT_KEY.c_str(), po::value<uint16_t>()->default_value(PORT_DEFAULT), "Catena service port")
            (DASHBOARD_PORT_KEY.c_str(), po::value<uint16_t>()->default_value(DASHBOARD_PORT_DEFAULT), "Port to serve connectionprops.xml")
            (DASHBOARD_TLS_ENABLED_KEY.c_str(), po::value<bool>()->default_value(DASHBOARD_TLS_ENABLED_DEFAULT)->implicit_value(true), "Use this to indicate to connection-props if tls_enabled")
            (DEFAULT_MAX_ARRAY_SIZE_KEY.c_str(), po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default max length for array and string params.")
            (DEFAULT_TOTAL_ARRAY_SIZE_KEY.c_str(), po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default total length for string array params.")
            (MAX_CONNECTIONS_KEY.c_str(), po::value<uint32_t>()->default_value(DEFAULT_MAX_CONNECTIONS), "Use this to define the total number of concurrent connections that can be made to a service.")
            (PRIVATE_CA_KEY.c_str(), po::value<bool>()->default_value(PRIVATE_CA_DEFAULT)->implicit_value(true), "Specify if using a private CA")
            (MUTUAL_AUTHC_KEY.c_str(), po::value<bool>()->default_value(MUTUAL_AUTHC_DEFAULT)->implicit_value(true), "Use this to require client to authenticate")
            (AUTHZ_KEY.c_str(), po::value<bool>()->default_value(AUTHZ_DEFAULT)->implicit_value(true), "Use OAuth token authorization")
            (SILENT_KEY.c_str(), po::value<bool>()->default_value(SILENT_DEFAULT)->implicit_value(true), "Use this to suppress all log output.")
            (LOG_LEVEL_KEY.c_str(), po::value<std::string>()->default_value(LOG_LEVEL_DEFAULT), "Minimum severity level of logs. Options are 'trace', 'debug', 'info', 'warning', 'error', and 'fatal'")
            (LOG_CONSOLE_KEY.c_str(), po::value<bool>()->default_value(LOG_CONSOLE_DEFAULT)->implicit_value(true), "Use console logging(stdout/stderr)")
            (LOG_FILE_KEY.c_str(), po::value<bool>()->default_value(LOG_FILE_DEFAULT)->implicit_value(true), "Use file logging")
            (LOG_SIZE_KEY.c_str(), po::value<double>()->default_value(LOG_SIZE_DEFAULT), "Max MB of log files before rotating. Must be greater than 0.")
            (LOG_COUNT_KEY.c_str(), po::value<int>()->default_value(LOG_COUNT_DEFAULT), "Max number of log files for this application before deleting old files. Includes the active file. Minimum value of 1")
            (LOG_MAX_SIZE_KEY.c_str(), po::value<double>()->default_value(LOG_MAX_SIZE_DEFAULT), "Convenience option. Derives count and size based on the max size in MB. Minimum value of 10.")
            (LOG_FINAL_ROTATION_KEY.c_str(), po::value<bool>()->default_value(LOG_FINAL_ROTATION_DEFAULT)->implicit_value(true), "Use this to archive the active log file upon teardown. Max number of archived files is 1 less than log count.")
            (LOG_APPEND_KEY.c_str(), po::value<bool>()->default_value(LOG_APPEND_DEFAULT)->implicit_value(true), "Use this to append to an existing, non-archived log file")
            ;
            
        std::set<std::string> allowedOptions;
        for (const auto& opt : configArgs.options()) {
            allowedOptions.insert(opt->long_name());
        }

        //Read values from command line and environment variables
        po::variables_map vars;
        po::store(po::parse_command_line(argc, argv, configArgs), vars);
        po::store(
            po::parse_environment(
                configArgs,
                [prefix, &allowedOptions](const std::string& env_name) -> std::string {
                    if (env_name.rfind(prefix, 0) != 0)
                        return "";
                    std::string option_name = env_name.substr(prefix.size());
                    std::transform(option_name.begin(), option_name.end(),
                                   option_name.begin(),
                                   [](unsigned char c) { return std::tolower(c); });

                    if (allowedOptions.count(option_name))
                        return option_name;
                    std::cout << "Ignoring environment variable " << env_name
                              << " since it does not correspond to a valid option" << std::endl;
                    return "";
                }),
            vars);
        
        
        po::notify(vars);
        
        // help message
        if (vars.count(HELP_KEY) > 0) {
            configArgs.print(std::cout);
            return std::pair<bool, int>(true, 0);
        }
        
        //Store values in config variables
        if (vars.count(SECURE_COMMS_KEY)) config::secure_comms = vars[SECURE_COMMS_KEY].as<std::string>();
        if (vars.count(CERTS_KEY)) config::certs = vars[CERTS_KEY].as<std::string>();
        if (vars.count(CERT_FILE_KEY)) config::cert_file = vars[CERT_FILE_KEY].as<std::string>();
        if (vars.count(KEY_FILE_KEY)) config::key_file = vars[KEY_FILE_KEY].as<std::string>();
        if (vars.count(CA_FILE_KEY)) config::ca_file = vars[CA_FILE_KEY].as<std::string>();
        if (vars.count(STATIC_ROOT_KEY)) config::static_root = vars[STATIC_ROOT_KEY].as<std::string>();
        if (vars.count(LOG_DIR_KEY)) config::log_dir = vars[LOG_DIR_KEY].as<std::string>();
        if (vars.count(DEFAULT_MAX_ARRAY_SIZE_KEY)) config::default_max_array_size = vars[DEFAULT_MAX_ARRAY_SIZE_KEY].as<uint32_t>();
        if (vars.count(DEFAULT_TOTAL_ARRAY_SIZE_KEY)) config::default_total_array_size = vars[DEFAULT_TOTAL_ARRAY_SIZE_KEY].as<uint32_t>();
        if (vars.count(MAX_CONNECTIONS_KEY)) config::max_connections = vars[MAX_CONNECTIONS_KEY].as<uint32_t>();
        if (vars.count(HOSTNAME_KEY)) config::hostname = vars[HOSTNAME_KEY].as<std::string>();
        if (vars.count(PORT_KEY)) config::port = vars[PORT_KEY].as<uint16_t>();
        if (vars.count(DASHBOARD_PORT_KEY)) config::dashboard_port = vars[DASHBOARD_PORT_KEY].as<uint16_t>();
        if (vars.count(DASHBOARD_TLS_ENABLED_KEY)) config::dashboard_tls_enabled = vars[DASHBOARD_TLS_ENABLED_KEY].as<bool>();
        if (vars.count(PRIVATE_CA_KEY)) config::private_ca = vars[PRIVATE_CA_KEY].as<bool>();
        if (vars.count(MUTUAL_AUTHC_KEY)) config::mutual_authc = vars[MUTUAL_AUTHC_KEY].as<bool>();
        if (vars.count(AUTHZ_KEY)) config::authz = vars[AUTHZ_KEY].as<bool>();
        if (vars.count(SILENT_KEY)) config::silent = vars[SILENT_KEY].as<bool>();
        if (vars.count(LOG_CONSOLE_KEY)) config::log_console = vars[LOG_CONSOLE_KEY].as<bool>();
        if (vars.count(LOG_FILE_KEY)) config::log_file = vars[LOG_FILE_KEY].as<bool>();
        if (vars.count(LOG_SIZE_KEY)) {
            config::log_size = vars[LOG_SIZE_KEY].as<double>();
            if (config::log_size <= 0) {
                std::cout << "WARNING: log_size is set below minimum of 0. Defualting to 10.0." << std::endl;
                config::log_size = config::LOG_SIZE_DEFAULT;
            }
        }
        if (vars.count(LOG_COUNT_KEY)) {
            config::log_count = vars[LOG_COUNT_KEY].as<int>();
            if (config::log_count < 1) {
                std::cout << "WARNING: log_count is set below the minimum of 1. Will be treated as log_count=1." << std::endl;
                config::log_count = 1;
            }
        }
        if (vars.count(LOG_LEVEL_KEY)){ 
            config::log_level = vars[LOG_LEVEL_KEY].as<std::string>();
            std::transform(config::log_level.begin(), config::log_level.end(), config::log_level.begin(), tolower);
            if (!log_levels.contains(config::log_level.c_str())) {
                std::cout << "WARNING: log_level {" << config::log_level << "} is invalid. ";
                std::cout << "Defaulting to " << config::LOG_LEVEL_DEFAULT << " instead." << std::endl;
                config::log_level = config::LOG_LEVEL_DEFAULT; 
            }
        }
        if (vars.count(LOG_MAX_SIZE_KEY)) {
            config::log_max_size = vars[LOG_MAX_SIZE_KEY].as<double>();
            if (config::log_max_size < 10) {
                std::cout << "WARNING: log_max_size is set below the minimum of 10. Ignoring log_max_size." << std::endl;
            }
            else if (!vars[LOG_MAX_SIZE_KEY].defaulted()) {
                if (!vars[LOG_SIZE_KEY].defaulted()) {
                    std::cout << "WARNING: log_size is set. Ignoring log_max_size." << std::endl;
                } else if (!vars[LOG_COUNT_KEY].defaulted()) {
                    std::cout << "WARNING: log_count is set. Ignoring log_max_size." << std::endl;
                } else {
                    config::log_count = config::log_max_size / 10;
                    config::log_size = config::log_max_size / config::log_count;
                }
            }
        }
        if (vars.count(LOG_FINAL_ROTATION_KEY)) {
            config::log_final_rotation = vars[LOG_FINAL_ROTATION_KEY].as<bool>();
            if (config::log_count == 1 && config::log_final_rotation) {
                std::cout << "WARNING: log_final_rotation is true and log_count == 1. No archiving will occur." << std::endl;
            }
        }
        if (vars.count(LOG_APPEND_KEY)) {
            config::log_append = vars[LOG_APPEND_KEY].as<bool>();
            if (config::log_final_rotation == true && config::log_append == true) {
                std::cout << "WARNING: log_final_rotation is true. No file will be left for subsequent runs to append to." << std::endl;
            }
        };

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::pair<bool, int>(true, 1);
    }

    return std::pair<bool, int>(false, 0);
}
