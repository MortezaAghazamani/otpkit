/**
 * @file base32.hpp
 * @brief RFC 4648 Base32 encode/decode (no padding on encode).
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace otpkit {

/**
 * @brief Encode bytes into a Base32 string.
 *
 * The output is **unpadded**, matching what Google Authenticator and
 * RFC 4226 test vectors use. The result is uppercase.
 *
 * @param data Raw bytes.
 * @return Base32 string.
 */
std::string base32Encode(const std::vector<std::uint8_t>& data);

/**
 * @brief Decode a Base32 string into bytes.
 *
 * Accepts both padded and unpadded input; whitespace and '=' are ignored.
 * Input is case-insensitive.
 *
 * @param s Base32 string.
 * @return Decoded bytes.
 * @throws std::invalid_argument if an invalid character is encountered.
 */
std::vector<std::uint8_t> base32Decode(const std::string& s);

/**
 * @brief Normalize user-entered Base32 input.
 *
 * - Converts to uppercase.
 * - Removes spaces, dashes, and trailing '=' padding.
 * - Rejects characters outside the Base32 alphabet.
 *
 * @throws std::invalid_argument on invalid characters.
 */
std::string normalizeBase32(const std::string& input);

} // namespace otpkit
