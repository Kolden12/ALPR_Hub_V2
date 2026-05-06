#include "sabre_crypto.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

std::string calculate_sabre_signature(const std::string& plate,
                                      const std::string& timestamp,
                                      double lat, double lon,
                                      const std::vector<uint8_t>& image_data) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    std::string metadata = plate + "|" + timestamp + "|" + std::to_string(lat) + "|" + std::to_string(lon) + "|";
    SHA256_Update(&sha256, metadata.c_str(), metadata.size());
    SHA256_Update(&sha256, image_data.data(), image_data.size());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
