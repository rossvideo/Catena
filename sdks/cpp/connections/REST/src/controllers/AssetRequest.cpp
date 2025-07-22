// connections/REST
#include <controllers/AssetRequest.h>
#include <fstream>
#include <filesystem>
#include <zlib.h>
#include <sys/stat.h>
#include <Authorization.h>
using catena::REST::AssetRequest;

// Initializes the object counter for GetParam to 0.
int AssetRequest::objectCounter_ = 0;

AssetRequest::AssetRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms} {
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

void AssetRequest::decompress(std::vector<uint8_t>& input, int windowBits) {
    z_stream zs{};
    if (inflateInit2_(&zs, windowBits, ZLIB_VERSION, sizeof(z_stream)) != Z_OK) {
        throw std::runtime_error("Failed to initialize decompression");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(input.data()));
    zs.avail_in = input.size();

    std::vector<uint8_t> output;
    output.resize(input.size() * 10);

    zs.next_out = output.data();
    zs.avail_out = output.size();

    if (inflate(&zs, Z_FINISH) != Z_STREAM_END) {
        inflateEnd(&zs);
        throw std::runtime_error("Decompression failed");
    }

    output.resize(zs.total_out);
    inflateEnd(&zs);

    input = output;
}

void AssetRequest::deflate_decompress(std::vector<uint8_t>& input) {
    decompress(input, MAX_WBITS);
}

void AssetRequest::gzip_decompress(std::vector<uint8_t>& input) {
    decompress(input, 16 + MAX_WBITS);
}

bool AssetRequest::get_last_write_time(const std::string& path, std::time_t& out_time) {
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0) {
        out_time = file_stat.st_mtime;
        return true;
    }
    return false;
}

void AssetRequest::extractPayload(const std::string& filePath) {
    std::vector<uint8_t> file_data(context_.jsonBody().begin(), context_.jsonBody().end());

    // Decompress if needed
    if (context_.fields("compression") == "GZIP") {
        DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] decompressing GZIP";
        gzip_decompress(file_data);
    } else if (context_.fields("compression") == "DEFLATE") {
        DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] decompressing DEFLATE";
        deflate_decompress(file_data);
    }

    // Ensure destination directory exists
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

    // Save to file
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::string error = "AssetRequest[" + std::to_string(objectId_) + "] failed to open file for writing: " + filePath;
        throw catena::exception_with_status(error, catena::StatusCode::INTERNAL);
    }
    file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
    file.close();
}

void AssetRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::DeviceComponent_ComponentParam ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    catena::ExternalObjectPayload obj;
    std::shared_ptr<catena::common::Authorizer> sharedAuthz;
    catena::common::Authorizer* authz;

    try {
        IDevice* dm = nullptr;
        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }

        if (context_.method() != Method_GET && context_.authorizationEnabled()) {
            // Authorizer throws an error if invalid jws token so no need to handle rc.
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }

        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);
        }

        // GET/asset
        else if (context_.method() == Method_GET) {
            // Locking device and parsing object data.
            DEBUG_LOG << "sending asset: " << context_.fqoid();
            std::string path = context_.EOPath();
            path.append(context_.fqoid());

            //check for any read access
            //TODO: move to BL
            if (!(authz->readAuthz(catena::common::Scopes_e::kOperate))) {   
                //do something that indicates that the user is not authorized to download the asset
                throw catena::exception_with_status("Not authorized to download asset", catena::StatusCode::PERMISSION_DENIED);
            }

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

            dm->getDownloadAssetRequest().emit(context_.fqoid(), authz);
        
        }

        // POST/asset
        else if (context_.method() == Method_POST) {
            DEBUG_LOG << "receiving asset: " << context_.fqoid();

            //Check if the user has write authorization in any scope other than monitoring
            //either authz have to be disabled or have write access to any scope other than monitor
            //TODO: move to BL
            if (!(authz->writeAuthz(catena::common::Scopes_e::kOperate)
                    || authz->writeAuthz(catena::common::Scopes_e::kConfig)
                    || authz->writeAuthz(catena::common::Scopes_e::kAdmin))) {
                throw catena::exception_with_status("Not authorized to POST asset", catena::StatusCode::PERMISSION_DENIED);
            }

            //examine authz if has write access in BL
            //if not, dont post
            //TODO: hook up business logic to handle asset upload
            dm->getUploadAssetRequest().emit(context_.fqoid(), authz);

            std::string filePath = context_.EOPath();
            filePath.append(context_.fqoid());

            // Check if the file exists
            // Placeholder till BL hook is implemented
            if (std::filesystem::exists(filePath)) {
                std::string found = "file: " + filePath + " already exists";
                DEBUG_LOG << found;
                throw catena::exception_with_status(found, catena::StatusCode::ALREADY_EXISTS);
            }
        
            extractPayload(filePath);
        
            DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] wrote file: " + filePath;
        
            // Set result status to OK
            rc = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);
        }
        
        // PUT/asset
        else if (context_.method() == Method_PUT) {
            DEBUG_LOG << "receiving asset: " << context_.fqoid();

            //Check if the user has write authorization in any scope other than monitoring
            //either authz have to be disabled or have write access to any scope other than monitor
            //TODO: move to BL
            if (!(authz->writeAuthz(catena::common::Scopes_e::kOperate)
                    || authz->writeAuthz(catena::common::Scopes_e::kConfig)
                    || authz->writeAuthz(catena::common::Scopes_e::kAdmin))) {
                throw catena::exception_with_status("Not authorized to PUT asset", catena::StatusCode::PERMISSION_DENIED);
            }

            //examine authz if has write access in BL
            //if not, dont post
            //TODO: hook up business logic to handle asset upload
            dm->getUploadAssetRequest().emit(context_.fqoid(), authz);

            std::string filePath = context_.EOPath();
            filePath.append(context_.fqoid());

            // Check if the file exists
            // Placeholder till BL hook is implemented
            if (!std::filesystem::exists(filePath)) {
                std::string notFound = "file: " + filePath + " not found";
                DEBUG_LOG << notFound;
                throw catena::exception_with_status(notFound, catena::StatusCode::NOT_FOUND);
            }
        
            extractPayload(filePath);
        
            DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] wrote file: " + filePath;
        
            // Set result status to OK
            rc = catena::exception_with_status("", catena::StatusCode::NO_CONTENT);
        }
        
        // DELETE/asset
        else if (context_.method() == Method_DELETE) {
            // TODO: Implement DELETE/asset
        }
        
        // ERROR
        else {
            rc = catena::exception_with_status("Invalid method", catena::StatusCode::INVALID_ARGUMENT);
        }
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
    if (context_.method() == Method_GET) {
        //For now we are sending the whole file in one go
        writer_.sendResponse(rc, obj);
    } else {
        // For POST, PUT and DELETE we do not return a message.
        writer_.sendResponse(rc);
    }

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "AssetRequest[" + std::to_string(objectId_) + "] for file: " + context_.fqoid() +" finished";
}
