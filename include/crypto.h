// crypto.h
#ifndef CRYPTO_H
#define CRYPTO_H

#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <sstream>
#include <string>
#include <vector>

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

    std::stringstream sha256;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        sha256 << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return sha256.str();
}

/// @brief Compute the SHA 256 Hash - Overload to Map Input Bytes to a vec<uchar> for OpenSSL
/// @param filePath
/// @return std::string
inline auto computeSHA256(const std::string &filePath) -> std::string {
    std::ifstream file(filePath, std::ifstream::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }
    std::vector<unsigned char> data(std::istreambuf_iterator<char>(file), {});
    return computeSHA256(data);
}

#endif // CRYPTO_H