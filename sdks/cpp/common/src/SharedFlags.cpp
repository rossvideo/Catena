
#include <cstdlib>
#include <SharedFlags.h>
#include <Device.h>

ABSL_FLAG(std::string, certs, "${HOME}/test_certs", "path/to/certs/files");
ABSL_FLAG(std::string, secure_comms, "off", "Specify type of secure comms, options are: \"off\", \"tls\"");
ABSL_FLAG(std::string, cert_file, "server.crt", "Specify the certificate file");
ABSL_FLAG(std::string, key_file, "server.key", "Specify the key file");
ABSL_FLAG(std::string, ca_file, "ca.crt", "Specify the CA file if using a private CA");
ABSL_FLAG(bool, private_ca, false, "Specify if using a private CA");
ABSL_FLAG(bool, mutual_authc, false, "use this to require client to authenticate");
ABSL_FLAG(bool, authz, false, "use OAuth token authorization");
ABSL_FLAG(std::string, static_root, getenv("HOME"), "Specify the directory to search for external objects");
ABSL_FLAG(uint32_t, default_max_array_size, catena::common::kDefaultMaxArrayLength, "use this to define the default max length for array and string params.");
ABSL_FLAG(uint32_t, default_total_array_size, catena::common::kDefaultMaxArrayLength, "use this to define the default total length for string array params.");
