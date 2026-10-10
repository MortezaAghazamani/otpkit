/**
 * @file sha1.hpp
 * @brief Self-contained SHA-1 implementation (FIPS 180-4).
 *
 * No external dependency; suitable for HMAC in HOTP/TOTP.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace otpkit {

/**
 * @brief Incremental SHA-1 hash calculator.
 *
 * Typical usage:
 * @code
 *   otpkit::SHA1 h;
 *   h.update(data, len);
 *   auto digest = h.finalize();
 * @endcode
 */
class SHA1 {
public:
    static constexpr std::size_t DIGEST_SIZE = 20; ///< Output size in bytes.
    static constexpr std::size_t BLOCK_SIZE  = 64; ///< Internal block size.

    /// Construct a fresh hasher.
    SHA1();

    /**
     * @brief Feed raw bytes into the hasher.
     * @param data Pointer to the buffer.
     * @param len  Number of bytes.
     */
    void update(const std::uint8_t* data, std::size_t len);

    /// Convenience overload for a byte vector.
    void update(const std::vector<std::uint8_t>& data);

    /**
     * @brief Finalize the hash and return the digest.
     * @return 20-byte digest.
     * @warning After calling finalize() the object is left in a
     *          valid but reset-like state; do not call update() again.
     */
    std::array<std::uint8_t, DIGEST_SIZE> finalize();

    /// One-shot helper: hash the whole buffer in one call.
    static std::array<std::uint8_t, DIGEST_SIZE>
    hash(const std::vector<std::uint8_t>& data);

private:
    void processBlock(const std::uint8_t* block);

    std::uint32_t h_[5];
    std::uint64_t totalLen_;
    std::uint8_t  buffer_[BLOCK_SIZE];
    std::size_t   bufferLen_;
};

} // namespace otpkit
