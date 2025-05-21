// connections/REST
#include <controllers/AssetRequest.h>
#include <fstream>
#include <filesystem>
#include <zlib.h>
using catena::REST::AssetRequest;

// Initializes the object counter for GetParam to 0.
int AssetRequest::objectCounter_ = 0;

AssetRequest::AssetRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

// Compress using zlib (deflate)
std::vector<uint8_t> compress(const std::vector<char>& input, int windowBits) {
    z_stream zs{};
    if (deflateInit2_(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream)) != Z_OK) {
        throw std::runtime_error("Failed to initialize compression");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
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
    return output;
}

std::vector<uint8_t> deflate_compress(const std::vector<char>& input) {
    return compress(input, MAX_WBITS);
}

std::vector<uint8_t> gzip_compress(const std::vector<char>& input) {
    return compress(input, 16 + MAX_WBITS);
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
        std::cout << "sending asset: " << context_.fields("oid") <<"\n";
        std::string path = context_.EOPath();
        path.append("/" + context_.fields("oid"));

        // Check if the file exists
        if (!std::filesystem::exists(path)) {
            std::string notFound = "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fields("oid") + " not found\n";
            std::cout << notFound;
            throw catena::exception_with_status(notFound, catena::StatusCode::NOT_FOUND);
        }

        // Read the file into a byte array
        std::ifstream file(path, std::ios::binary);
        std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Set the payload encoding and compress the data accordingly
        if (context_.fields("compression") == "GZIP") {
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::GZIP);
            std::vector<uint8_t> compressed = gzip_compress(file_data);
            obj.mutable_payload()->set_payload(compressed.data(), compressed.size());

        } else if (context_.fields("compression") == "DEFLATE") {
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::DEFLATE);
            std::vector<uint8_t> compressed = deflate_compress(file_data);
            obj.mutable_payload()->set_payload(compressed.data(), compressed.size());

        } else {
            obj.mutable_payload()->set_payload_encoding(catena::DataPayload::UNCOMPRESSED);
            obj.mutable_payload()->set_payload(file_data.data(), file_data.size());
        }
        
        //Set cacheable
        obj.set_cachable(true);

        // Set the metadata
        auto metadata = obj.mutable_payload()->mutable_metadata();
        //metadata->insert({"content-type", determineContentType(path)});
        metadata->insert({"filename", std::filesystem::path(path).filename().string()});
        metadata->insert({"size", std::to_string(file_data.size())});
        metadata->insert({"last-modified", std::to_string(std::filesystem::last_write_time(path).time_since_epoch().count())});

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
    dm_.getAssetRequest().emit(context_.fields("oid"), objectId_);
}

void AssetRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fields("oid") +" finished\n";
}
