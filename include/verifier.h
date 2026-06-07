#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "logger.h"
#include <openssl/evp.h>

class Verifier {
public:
    // Compute SHA-256 of file and compare with expected hex string.
    bool verify_sha256(const std::string& file_path,
                       const std::string& expected_hex) const {
        std::string actual = compute_sha256(file_path);
        if (actual.empty()) return false;

        if (actual != expected_hex) {
            Logger::error("SHA-256 mismatch:");
            Logger::error("  expected: " + expected_hex);
            Logger::error("  actual  : " + actual);
            return false;
        }
        return true;
    }

private:
    std::string compute_sha256(const std::string& path) const {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            Logger::error("Cannot open file for checksum: " + path);
            return "";
        }

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) return "";

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        char buf[8192];
        while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx, buf, static_cast<size_t>(file.gcount())) != 1) {
                EVP_MD_CTX_free(ctx);
                return "";
            }
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int  hash_len = 0;
        if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
        EVP_MD_CTX_free(ctx);

        std::ostringstream oss;
        for (unsigned int i = 0; i < hash_len; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        return oss.str();
    }
};
