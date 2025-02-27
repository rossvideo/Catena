/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.
*/
//

/**
 * @file Authorization.cpp
 * @brief Implements the Authorizer class
 * @author John Danen
 * @date 2024-12-06
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <Authorization.h>

using catena::common::Authorizer;

// initialize the disabled authorization object
Authorizer Authorizer::kAuthzDisabled;

Authorizer::Authorizer(const std::string& JWSToken, const std::string& keycloakServer, const std::string& realm) {
    // Fetching and parsing the jwks from authz server.
    std::string issuer = "https://" + keycloakServer + "/realms/" + realm + "/protocol/openid-connect/certs";
    std::string jwks = fetchJWKS(issuer);
    std::map<std::string, std::string> jwksInfo;
    parseJWKS(jwks, jwksInfo);
    std::string publicKey = ES256Key(jwksInfo["x"], jwksInfo["y"]);
    try {
        // Decoding token, constructing verifier, and verifying the token.
        jwt::decoded_jwt<jwt::traits::kazuho_picojson> decodedToken = jwt::decode(JWSToken);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::es256(publicKey));
            //.allow_algorithm(jwt::algorithm::rs256(PUBLIC_KEY, "", "", ""));
            // .with_type("at+jwt")
            // // .with_issuer("https://catena.rossvideo.com")
            // // .with_audience("https://catena.rossvideo.com")
            // .leeway(300); // 5 minutes of leeway for nbf and exp.
        // if (jwksInfo["alg"] == "ES256") {
        //     if (jwksInfo["crv"] == "P-256") {
        //         verifier.allow_algorithm(jwt::algorithm::es256(publicKey));
        //     }
        // }
        verifier.verify(decodedToken); // Throws error if invalid.

        // Extracting scopes from the decoded token's claims.
        auto claims = decodedToken.get_payload_json();
        for (picojson::value::object::const_iterator it = claims.begin(); it != claims.end(); ++it) {
            if (it->first == "scope"){
                std::string scopeClaim = it->second.get<std::string>();
                std::istringstream iss(scopeClaim);
                while (std::getline(iss, scopeClaim, ' ')) {
                    clientScopes_.push_back(scopeClaim);
                }
            }
        }
        
    // Catch error thrown by verifier.verify().
    } catch (jwt::error::token_verification_exception err) {
        throw catena::exception_with_status(err.what(), catena::StatusCode::UNAUTHENTICATED);}
    // Catch any other errors (probably from jwt::decode())
    // } catch (...) {
    //     throw catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // }
}

size_t Authorizer::curlWriteFunction(char* contents, size_t size, size_t numMembers, void* output) {
    size_t realSize = size * numMembers;
    std::string *str = (std::string *)output;
    str->append(contents, size * numMembers);
    return realSize;
}

std::string Authorizer::fetchJWKS(const std::string& url) {
    // Initializing CURL.
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw catena::exception_with_status("Failed to initialize curl", catena::StatusCode::INTERNAL);
    }
    // Setting up CURL options.
    std::string jwks;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&jwks);
    // Running CURL and returning the jwks.
    CURLcode res =  curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        throw catena::exception_with_status("Failed to fetch JWKS", catena::StatusCode::INTERNAL);
    }
    return jwks;
}

void Authorizer::parseJWKS(const std::string& jwks, std::map<std::string, std::string>& jwksMap, std::string alg) {
    try {
        auto rc = catena::exception_with_status("", catena::StatusCode::OK);
        picojson::value jsonData;
        picojson::parse(jsonData, jwks);   
        picojson::value keys = jsonData.get("keys");
        for (auto key : keys.get<picojson::array>()) {
            if (key.get("alg").get<std::string>() == alg) {
                // Algorithm found, extracting its fields.
                for (auto [field, value] : key.get<picojson::object>()) {
                    jwksMap[field] = value.get<std::string>();
                }
                break;
            }
        }
    } catch (...) {
        throw catena::exception_with_status("Failed to parse JWKS from authz server", catena::StatusCode::INTERNAL);
    }
    // Did not find specified algorithm in JWKS.
    if (jwksMap.empty()) {
        throw catena::exception_with_status("Authz server does allow the algorithm " + alg, catena::StatusCode::UNAUTHENTICATED);
    }
}

bool Authorizer::hasAuthz(const std::string& scope) const {
    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    if (std::find(clientScopes_.begin(), clientScopes_.end(), scope) == clientScopes_.end()) {
        return false;
    }
    return true;
}

Authorizer::decodedB64Str Authorizer::base64UrlDecode(const std::string& input) {
    // Replace characters that are not allowed in base64url and add padding.
    std::string base64 = input;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    while (base64.size() % 4) { base64 += '='; }
    // Decode the base64url string.
    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    EVP_DecodeInit(ctx);
    decodedB64Str decoded(base64.size());
    int outLen = decoded.size(), inLen = base64.size();
    EVP_DecodeUpdate(ctx, decoded.data(), &outLen, (unsigned char*)base64.data(), inLen);
    decoded.resize(outLen);
    EVP_ENCODE_CTX_free(ctx);
    return decoded;
}

std::string Authorizer::ES256Key(const std::string& x, const std::string& y) {
    // Decoding x and y from base64URL and converting to BIGNUM.
    decodedB64Str xD = base64UrlDecode(x);
    decodedB64Str yD = base64UrlDecode(y); 
    BIGNUM* xBN = BN_bin2bn(xD.data(), xD.size(), NULL);
    BIGNUM* yBN = BN_bin2bn(yD.data(), yD.size(), NULL);
    
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    EC_POINT* point = EC_POINT_new(group);
    EC_POINT_set_affine_coordinates(group, point, xBN, yBN, NULL);

    // std::string encodedStr = std::string(xD.begin(), xD.end()) + std::string(yD.begin(), yD.end());
    unsigned char* encodedPoint = NULL;
    size_t encodedLen = EC_POINT_point2buf(group, point, POINT_CONVERSION_UNCOMPRESSED, &encodedPoint, NULL);
    //std::cerr<<std::string((const char*)encodedPoint)<<"\n";
    
    // int EVP_PKEY_CTX_set_ec_paramgen_curve_nid(EVP_PKEY_CTX *ctx, int nid);


    EVP_PKEY* pkey = EVP_PKEY_new();
    // SET PARAMS
    // OSSL_PARAM *EC_GROUP_to_params(const EC_GROUP *group, OSSL_LIB_CTX *libctx, const char *propq, BN_CTX *bnctx);
    // int EVP_PKEY_set_params(EVP_PKEY *pkey, OSSL_PARAM params[]);
    OSSL* params = EC_GROUP_to_params(group, NULL, NULL, NULL);
    EVP_PKEY_set_params(pkey, params);
    EVP_PKEY_set1_encoded_public_key(pkey, encodedPoint, encodedLen);

    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, pkey);

    char* pemData;
    long len = BIO_get_mem_data(bio, &pemData);
    std::string publicKey(pemData, len);
    // Freeing memory and returning public key.
    EVP_PKEY_free(pkey);
    EC_POINT_free(point);
    EC_GROUP_free(group);
    BN_free(xBN);
    BN_free(yBN);
    BIO_free(bio);
    OPENSSL_free(encodedPoint);
    publicKey.erase(std::remove(publicKey.begin(), publicKey.end(), '\n'), publicKey.cend());
    return publicKey;


    // EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    // EVP_PKEY_keygen_init(pctx);
    // // Setting the EC curve.
    // EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1);
    // point

    // // Generating the EC key.
    // EVP_PKEY* pkey = NULL;
    // EVP_PKEY_keygen(pctx, &pkey);
    // EVP_PKEY_CTX_free(pctx);

    // EC_KEY* eckey = EVP_PKEY_get1_EC_KEY(pkey);
    // EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    // EC_POINT* point = EC_POINT_new(group);
    // EC_POINT_set_affine_coordinates(group, point, xBN, yBN, NULL);

    // EVP_PKEY* pubkey = EVP_PKEY_new();
    // EVP_MD_CTX_set_pkey_ctx


    // return "";

    // // OLD CODE
    // EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    // if (!ecKey) {
    //     throw catena::exception_with_status("Failed to create EC_KEY", catena::StatusCode::INTERNAL);
    // }
    // // Converting x and y to BIGNUM and using them to create a point.
    // const EC_GROUP* group = EC_KEY_get0_group(ecKey);
    // BIGNUM* xBN = BN_bin2bn(xB64.data(), xB64.size(), NULL);
    // BIGNUM* yBN = BN_bin2bn(yB64.data(), yB64.size(), NULL);
    // EC_POINT* point = EC_POINT_new(group);
    // if (!EC_POINT_set_affine_coordinates(group, point, xBN, yBN, NULL)) {
    //     throw catena::exception_with_status("Failed to set affine coordinates", catena::StatusCode::INTERNAL);
    // }
    // // Using point to find the public key and writing in PEM format to buffer.
    // EC_KEY_set_public_key(ecKey, point);
    // BIO* bio = BIO_new(BIO_s_mem());
    // if (!PEM_write_bio_EC_PUBKEY(bio, ecKey)) {
    //     throw catena::exception_with_status("Failed to write public key to buffer", catena::StatusCode::INTERNAL);
    // }
    // // Converting buffer to string and storing it in publicKey.
    // char* pemData;
    // long len = BIO_get_mem_data(bio, &pemData);
    // std::string publicKey(pemData, len);
    // // Freeing memory and returning public key.
    // BIO_free(bio);
    // EC_KEY_free(ecKey);
    // EC_POINT_free(point);
    // BN_free(xBN);
    // BN_free(yBN);
    // publicKey.erase(std::remove(publicKey.begin(), publicKey.end(), '\n'), publicKey.cend());
    // return publicKey;
}

/**
 * @brief Check if the client has read authorization
 * @return true if the client has read authorization
 */
bool Authorizer::readAuthz(const IParam& param) const {
    const std::string& scope = param.getScope();
    return hasAuthz(scope) || writeAuthz(param);
}

bool Authorizer::readAuthz(const ParamDescriptor& pd) const {
    const std::string& scope = pd.getScope();
    return hasAuthz(scope) || writeAuthz(pd);
}

/**
 * @brief Check if the client has write authorization
 * @return true if the client has write authorization
 */
bool Authorizer::writeAuthz(const IParam& param) const {
    if (param.readOnly()) {
        return false;
    }

    const std::string scope = param.getScope() + ":w";
    return hasAuthz(scope);
}

bool Authorizer::writeAuthz(const ParamDescriptor& pd) const {
    if (pd.readOnly()) {
        return false;
    }

    const std::string scope = pd.getScope() + ":w";
    return hasAuthz(scope);
}
