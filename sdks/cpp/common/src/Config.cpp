#include <Config.h>
#include <Device.h>
#include <boost/program_options.hpp>
#include <Logger.h>

namespace po = boost::program_options;
using namespace catena::common;

std::pair<bool, int> config::initConfigVariables(int argc, char* argv[], std::string prefix) {
    try {
        //Declare options
        const char* home = (getenv("HOME") != nullptr) ? getenv("HOME") : CATENA_STATIC_ROOT.c_str(); // TODO: Add windows functionality
        po::options_description configArgs("Allowed options");
        configArgs.add_options()
            (CATENA_HELP_NAME.c_str(), "Get help message")
            (CATENA_SECURE_COMMS_NAME.c_str(), po::value<std::string>()->default_value(CATENA_SECURE_COMMS), "Specify type of secure comms, options are: \"off\", \"tls\"")
            (CATENA_CERTS_NAME.c_str(), po::value<std::string>()->default_value(CATENA_CERTS), "path/to/certs/files")
            (CATENA_CERT_FILE_NAME.c_str(), po::value<std::string>()->default_value(CATENA_CERT_FILE), "Specify the certificate file")
            (CATENA_KEY_FILE_NAME.c_str(), po::value<std::string>()->default_value(CATENA_KEY_FILE), "Specify the key file")
            (CATENA_CA_FILE_NAME.c_str(), po::value<std::string>()->default_value(CATENA_CA_FILE), "Specify the CA file if using a private CA")
            (CATENA_STATIC_ROOT_NAME.c_str(), po::value<std::string>()->default_value(home), "Specify the directory to search for external objects")
            (CATENA_LOG_DIR_NAME.c_str(), po::value<std::string>()->default_value(LOG_DIR), "Specify the directory for log files.")
            (CATENA_PORT_NAME.c_str(), po::value<uint16_t>()->default_value(CATENA_PORT), "Catena service port")
            (CATENA_DEFAULT_MAX_ARRAY_SIZE_NAME.c_str(), po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default max length for array and string params.")
            (CATENA_DEFAULT_TOTAL_ARRAY_SIZE_NAME.c_str(), po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default total length for string array params.")
            (CATENA_MAX_CONNECTIONS_NAME.c_str(), po::value<uint32_t>()->default_value(DEFAULT_MAX_CONNECTIONS), "Use this to define the total number of concurrent connections that can be made to a service.")
            (CATENA_PRIVATE_CA_NAME.c_str(), po::value<bool>()->default_value(false)->implicit_value(true), "Specify if using a private CA")
            (CATENA_MUTUAL_AUTHC_NAME.c_str(), po::value<bool>()->default_value(false)->implicit_value(true), "Use this to require client to authenticate")
            (CATENA_AUTHZ_NAME.c_str(), po::value<bool>()->default_value(false)->implicit_value(true), "Use OAuth token authorization")
            (CATENA_SILENT_NAME.c_str(), po::value<bool>()->default_value(false)->implicit_value(true), "Use this to suppress all log output.")
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
        if (vars.count(CATENA_HELP_NAME) > 0) {
            configArgs.print(std::cout);
            return std::pair<bool, int>(true, 0);
        }
        
        //Store values in config variables
        if (vars.count(CATENA_SECURE_COMMS_NAME)) config::secure_comms = vars[CATENA_SECURE_COMMS_NAME].as<std::string>();
        if (vars.count(CATENA_CERTS_NAME)) config::certs = vars[CATENA_CERTS_NAME].as<std::string>();
        if (vars.count(CATENA_CERT_FILE_NAME)) config::cert_file = vars[CATENA_CERT_FILE_NAME].as<std::string>();
        if (vars.count(CATENA_KEY_FILE_NAME)) config::key_file = vars[CATENA_KEY_FILE_NAME].as<std::string>();
        if (vars.count(CATENA_CA_FILE_NAME)) config::ca_file = vars[CATENA_CA_FILE_NAME].as<std::string>();
        if (vars.count(CATENA_STATIC_ROOT_NAME)) config::static_root = vars[CATENA_STATIC_ROOT_NAME].as<std::string>();
        if (vars.count(CATENA_LOG_DIR_NAME)) config::log_dir = vars[CATENA_LOG_DIR_NAME].as<std::string>();
        if (vars.count(CATENA_DEFAULT_MAX_ARRAY_SIZE_NAME)) config::default_max_array_size = vars[CATENA_DEFAULT_MAX_ARRAY_SIZE_NAME].as<uint32_t>();
        if (vars.count(CATENA_DEFAULT_TOTAL_ARRAY_SIZE_NAME)) config::default_total_array_size = vars[CATENA_DEFAULT_TOTAL_ARRAY_SIZE_NAME].as<uint32_t>();
        if (vars.count(CATENA_MAX_CONNECTIONS_NAME)) config::max_connections = vars[CATENA_MAX_CONNECTIONS_NAME].as<uint32_t>();
        if (vars.count(CATENA_PORT_NAME)) config::port = vars[CATENA_PORT_NAME].as<uint16_t>();
        if (vars.count(CATENA_PRIVATE_CA_NAME)) config::private_ca = vars[CATENA_PRIVATE_CA_NAME].as<bool>();
        if (vars.count(CATENA_MUTUAL_AUTHC_NAME)) config::mutual_authc = vars[CATENA_MUTUAL_AUTHC_NAME].as<bool>();
        if (vars.count(CATENA_AUTHZ_NAME)) config::authz = vars[CATENA_AUTHZ_NAME].as<bool>();
        if (vars.count(CATENA_SILENT_NAME)) config::silent = vars[CATENA_SILENT_NAME].as<bool>();

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::pair<bool, int>(true, 1);
    }

    return std::pair<bool, int>(false, 0);
}
