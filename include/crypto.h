// crypto.h
#ifndef CRYPTO_H
#define CRYPTO_H

#include "fs.h"
#include <openssl/evp.h>
#include <string>
#include <vector>

#pragma region CRYPTOGRAPHY_IMPL

/// @brief Compute the SHA 256 Hash
/// @param data
/// @return std::string
inline auto computeSHA256(const std::vector<unsigned char> &data) -> std::string {
    EVP_MD_CTX *mdContext = EVP_MD_CTX_new();
    if (mdContext == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdContext, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdContext);
        throw std::runtime_error("Failed to initialize EVP Digest");
    }

    if (EVP_DigestUpdate(mdContext, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdContext);
        throw std::runtime_error("Failed to update digest");
    }

    std::vector<unsigned char> hash(EVP_MD_size(EVP_sha256()));

    unsigned int lengthOfHash = 0;

    if (EVP_DigestFinal_ex(mdContext, hash.data(), &lengthOfHash) != 1) {
        EVP_MD_CTX_free(mdContext);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdContext);

    std::string result;

    llvm::raw_string_ostream rso(result);
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        rso << llvm::format_hex_no_prefix(hash[i], 2);
    }
    return rso.str();
}

/// @brief Compute the SHA 256 Hash - Overload to Map Input Bytes to a vec<uchar> for OpenSSL
/// @param filePath
/// @return std::string
inline auto computeSHA256(const std::string &filePath) -> std::string {
    auto fileContentOrErr = readFileUChar(filePath);
    if (!fileContentOrErr) {
        llvm::errs() << "Error: " << llvm::toString(fileContentOrErr.takeError()) << "\n";
        return "";
    }
    return computeSHA256(fileContentOrErr.get());
}

#pragma endregion

#endif // CRYPTO_H