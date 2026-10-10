/**
 * @file hmac.hpp
 * @brief Generic HMAC on top of SHA1/SHA256/SHA512 (RFC 2104).
 */
#pragma once

#include "otpkit/config.hpp"

#include <cstdint>
#include <vector>

namespace otpkit {

/**
 * @brief Compute HMAC for the given algorithm.
 *
 * @param algo    Hash algorithm to use.
 * @param key     Raw key bytes.
 * @param message Message bytes.
 * @return HMAC digest whose length equals the underlying hash digest size.
 */
std::vector<std::uint8_t> hmac(Algorithm algo,
                               const std::vector<std::uint8_t>& key,
                               const std::vector<std::uint8_t>& message);

} // namespace otpkit
