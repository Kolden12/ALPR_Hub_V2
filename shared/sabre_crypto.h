#ifndef SABRE_CRYPTO_H
#define SABRE_CRYPTO_H

#include <stdint.h>
#include <string>
#include <vector>

/**
 * Generates SHA-256 for chain-of-custody.
 * signature = SHA256(plate_text + timestamp + gps_lat + gps_long + raw_image_bytes)
 */
std::string calculate_sabre_signature(const std::string& plate,
                                      const std::string& timestamp,
                                      double lat, double lon,
                                      const std::vector<uint8_t>& image_data);

#endif
