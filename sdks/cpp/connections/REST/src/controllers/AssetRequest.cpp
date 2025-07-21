// connections/REST
#include <controllers/AssetRequest.h>
#include <fstream>
#include <filesystem>
#include <zlib.h>
#include <sys/stat.h>
using catena::REST::AssetRequest;

// Initializes the object counter for GetParam to 0.
int AssetRequest::objectCounter_ = 0;

AssetRequest::AssetRequest(ICatenaServiceImpl *service, tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    service_{service}, socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

// Compress using zlib (deflate)
void AssetRequest::compress(std::vector<uint8_t>& input, int windowBits) {
    z_stream zs{};
    if (deflateInit2_(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream)) != Z_OK) {
        throw std::runtime_error("Failed to initialize compression");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(input.data()));
    zs.avail_in = input.size();

    std::vector<uint8_t> output;
    output.resize(compressBound(input.size()));

    zs.next_out = output.data();
    zs.avail_out = output.size();

    if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&zs);
        throw std::runtime_error("Compression failed");
    }

    output.resize(zs.total_out);
    deflateEnd(&zs);

    input = output;
}

void AssetRequest::deflate_compress(std::vector<uint8_t>& input) {
    compress(input, MAX_WBITS);
}

void AssetRequest::gzip_compress(std::vector<uint8_t>& input) {
    compress(input, 16 + MAX_WBITS);
}

bool AssetRequest::get_last_write_time(const std::string& path, std::time_t& out_time) {
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0) {
        out_time = file_stat.st_mtime;
        return true;
    }
    return false;
}

void AssetRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::DeviceComponent_ComponentParam ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    catena::ExternalObjectPayload obj;

    try {
        IDevice* dm = nullptr;
        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }

        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);
        }

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
        DEBUG_LOG << "sending asset: " << context_.fqoid();
        std::string path = context_.EOPath();
        path.append(context_.fqoid());

        // Check if the file exists
        if (!std::filesystem::exists(path)) {
            std::string notFound = "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fqoid() + " not found";
            DEBUG_LOG << notFound;
            throw catena::exception_with_status(notFound, catena::StatusCode::NOT_FOUND);
        }

        // Read the file into a byte array
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::string error = "AssetRequest[" + std::to_string(objectId_) + "] failed to open file: " + context_.fqoid() + "\n";
            throw catena::exception_with_status(error, catena::StatusCode::INTERNAL);
        }
        std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (file_data.empty()) {
            std::string error = "AssetRequest[" + std::to_string(objectId_) + "] file is empty: " + context_.fqoid() + "\n";
            throw catena::exception_with_status(error, catena::StatusCode::INVALID_ARGUMENT);
        }

        // Set the payload encoding and compress the data accordingly
        if (context_.fields("compression") == "GZIP") {
            DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] using GZIP compression";
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::GZIP);
            gzip_compress(file_data);

        } else if (context_.fields("compression") == "DEFLATE") {
            DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] using DEFLATE compression";
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::DEFLATE);
            deflate_compress(file_data);

        } else {
            DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] using UNCOMPRESSED compression";
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::UNCOMPRESSED);
        }
        obj.mutable_payload()->set_payload(file_data.data(), file_data.size());
        
        //Set cacheable
        obj.set_cachable(true);

        // Set the metadata
        auto metadata = obj.mutable_payload()->mutable_metadata();

        metadata->insert({"filename", std::filesystem::path(path).filename().string()});
        metadata->insert({"size", std::to_string(file_data.size())});
        
        std::time_t modified_time;
        if (get_last_write_time(path, modified_time)) {
            metadata->insert({"last-modified", std::asctime(std::localtime(&modified_time))});
        }
        else {
            metadata->insert({"last-modified", "unknown"});
        }

        // Calculate SHA-256 digest
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digest_len;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(mdctx, file_data.data(), file_data.size());
        EVP_DigestFinal_ex(mdctx, digest, &digest_len);
        EVP_MD_CTX_free(mdctx);

        // Set the digest using the digest array
        obj.mutable_payload()->set_digest(digest, digest_len);

        dm->getAssetRequest().emit(context_.fqoid());
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::filesystem::filesystem_error& e) {
        std::string error = "AssetRequest[" + std::to_string(objectId_) + "] filesystem error: " + std::string(e.what()) + "\n";
        rc = catena::exception_with_status(error, catena::StatusCode::INTERNAL);
    } catch (const std::exception& e) {
        std::string error = "AssetRequest[" + std::to_string(objectId_) + "] error: " + std::string(e.what()) + "\n";
        rc = catena::exception_with_status(error, catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, obj);

    //For now we are sending the whole file in one go
    DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] sent";

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fqoid() +" finished";
}
