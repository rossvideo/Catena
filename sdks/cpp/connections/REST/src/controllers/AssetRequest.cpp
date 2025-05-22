
// connections/REST
#include <controllers/AssetRequest.h>
#include <fstream>
#include <filesystem>
using catena::REST::AssetRequest;

// Initializes the object counter for GetParam to 0.
int AssetRequest::objectCounter_ = 0;

AssetRequest::AssetRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void AssetRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::DeviceComponent_ComponentParam ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    catena::ExternalObjectPayload obj;

    try {
        // Creating authorizer.
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
        // Locking device and parsing object data.
        std::cout << "sending asset: " << context_.fqoid() <<"\n";
        std::string path = context_.EOPath();
        path.append(context_.fqoid());

        // Check if the file exists
        if (!std::filesystem::exists(path)) {
            std::string notFound = "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fqoid() + " not found\n";
            std::cout << notFound;
            throw catena::exception_with_status(notFound, catena::StatusCode::NOT_FOUND);
        }
        // Read the file into a byte array
        std::ifstream file(path, std::ios::binary);
        std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        obj.mutable_payload()->set_payload(file_data.data(), file_data.size()); 

        //For now we are sending the whole file in one go
        std::cout << "AssetRequest[" + std::to_string(objectId_) + "] sent\n";
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, obj);
}

void AssetRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fqoid() +" finished\n";
}
