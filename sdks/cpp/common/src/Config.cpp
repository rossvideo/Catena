#include <Config.h>
#include <Device.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace catena::common;

std::pair<boost::program_options::options_description, bool> config::initConfigVariables(int argc, char* argv[], std::string serviceType, std::string prefix) {
    //Declare options
    po::options_description configArgs("Allowed options");
    configArgs.add_options()
        ("help,h", "Get help message")
        ("secure_comms", po::value<std::string>()->default_value("off"), "Specify type of secure comms, options are: \"off\", \"tls\"")
        ("cert_file", po::value<std::string>()->default_value("server.crt"), "Specify the certificate file")
        ("key_file", po::value<std::string>()->default_value("server.key"), "Specify the key file")
        ("ca_file", po::value<std::string>()->default_value("ca.crt"), "Specify the CA file if using a private CA")
        ("log_dir", po::value<std::string>()->default_value(LOG_DIR), "Specify the directory for log files.")
        ("default_max_array_size", po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default max length for array and string params.")
        ("default_total_array_size", po::value<uint32_t>()->default_value(kDefaultMaxArrayLength), "Use this to define the default total length for string array params.")
        ("max_connections", po::value<uint32_t>()->default_value(DEFAULT_MAX_CONNECTIONS), "Use this to define the total number of concurrent connections that can be made to a service.")
        ("private_ca", po::value<bool>()->default_value(false)->implicit_value(true), "Specify if using a private CA")
        ("mutual_authc", po::value<bool>()->default_value(false)->implicit_value(true), "Use this to require client to authenticate")
        ("authz", po::value<bool>()->default_value(false)->implicit_value(true), "Use OAuth token authorization")
        ("silent", po::value<bool>()->default_value(false)->implicit_value(true), "Use this to suppress all log output.")
        ;
    char* home = getenv("HOME");
    if (home != nullptr) {
        configArgs.add_options()("certs", po::value<std::string>()->default_value("${HOME}/test_certs"), "path/to/certs/files");
        configArgs.add_options()("static_root", po::value<std::string>()->default_value(home), "Specify the directory to search for external objects");
    } else {
        throw std::runtime_error("\"HOME\" environment variable not found. Required to initialize static_root and certs default values.");
    }
    if (serviceType == "REST") {
        configArgs.add_options()("port", po::value<uint16_t>()->default_value(443), "Catena REST service port");
    } else if (serviceType == "GRPC") {
        configArgs.add_options()("port", po::value<uint16_t>()->default_value(6254), "Catena gRPC service port");
    }

    //Read values from command line and environment variables
    po::variables_map vars;
    po::store(po::parse_command_line(argc, argv, configArgs), vars);
    po::notify(vars);
    po::store(po::parse_environment(configArgs, prefix), vars);
    po::notify(vars);

    // help message
    bool help = false;
    if (vars.count("help")) {
        help = true;
    }

    //Store values in config variables
    if (vars.count("certs")) config::certs = vars["certs"].as<std::string>();
    if (vars.count("secure_comms")) config::secure_comms = vars["secure_comms"].as<std::string>();
    if (vars.count("cert_file")) config::cert_file = vars["cert_file"].as<std::string>();
    if (vars.count("key_file")) config::key_file = vars["key_file"].as<std::string>();
    if (vars.count("ca_file")) config::ca_file = vars["ca_file"].as<std::string>();
    if (vars.count("static_root")) config::static_root = vars["static_root"].as<std::string>();
    if (vars.count("log_dir")) config::log_dir = vars["log_dir"].as<std::string>();
    if (vars.count("default_max_array_size")) config::default_max_array_size = vars["default_max_array_size"].as<uint32_t>();
    if (vars.count("default_total_array_size")) config::default_total_array_size = vars["default_total_array_size"].as<uint32_t>();
    if (vars.count("max_connections")) config::max_connections = vars["max_connections"].as<uint32_t>();
    if (vars.count("port")) config::port = vars["port"].as<uint16_t>();
    if (vars.count("private_ca")) config::private_ca = vars["private_ca"].as<bool>();
    if (vars.count("mutual_authc")) config::mutual_authc = vars["mutual_authc"].as<bool>();
    if (vars.count("authz")) config::authz = vars["authz"].as<bool>();
    if (vars.count("silent")) config::silent = vars["silent"].as<bool>();

    return std::pair<boost::program_options::options_description, bool>(configArgs, help);
}
