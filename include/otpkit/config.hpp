/**
 * @file config.hpp
 * @brief Common configuration types shared by HOTP and TOTP.
 *
 * @copyright MIT License
 */
#pragma once

#include <cstdint>
#include <string>

namespace otpkit {

/**
 * @brief Hash algorithm used for HMAC-based OTP generation.
 *
 * Google Authenticator and most consumer apps use SHA1. SHA256 and SHA512
 * are supported by RFC 6238 and by some enterprise providers.
 */
enum class Algorithm {
    SHA1,   ///< HMAC-SHA1  (20-byte digest, most compatible)
    SHA256, ///< HMAC-SHA256 (32-byte digest)
    SHA512  ///< HMAC-SHA512 (64-byte digest)
};

/**
 * @brief Algorithm-only configuration.
 *
 * This struct intentionally contains **only** parameters that affect the
 * mathematical output of the OTP. Display labels (issuer, account name) are
 * passed separately to @ref Totp::otpauthUri.
 */
struct Config {
    /// HMAC hash algorithm.
    Algorithm algorithm = Algorithm::SHA1;

    /// Number of decimal digits in the produced code (6-8 typical).
    int digits = 6;

    /// Time step in seconds. Used by TOTP only; ignored by HOTP.
    int period = 30;
};

/**
 * @brief Convert an Algorithm enum to its canonical string name ("SHA1" etc.).
 * @param algo The algorithm.
 * @return A null-terminated static string.
 */
const char* algorithmName(Algorithm algo) noexcept;

} // namespace otpkit
