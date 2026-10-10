/**
 * @file sha256.hpp
 * @brief Self-contained SHA-256 implementation (FIPS 180-4).
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace otpkit {

/**
 * @brief Incremental SHA-256 hasher.
 */
class SHA256 {
public:
    static constexpr std::size_t DIGEST_SIZE = 32; ///< Output size in bytes.
    static constexpr std::size_t BLOCK_SIZE  = 64; ///< Internal block size.

    SHA256();

    void update(const std::uint8_t* data, std::size_t len);

    std::array<std::uint8_t, DIGEST_SIZE> finalize();

    static std::array<std::uint8_t, DIGEST_SIZE>
    hash(const std::vector<std::uint8_t>& data);

private:
    void processBlock(const std::uint8_t* block);

    std::uint32_t h_[8];
    std::uint64_t totalLen_;
    std::uint8_t  buffer_[BLOCK_SIZE];
    std::size_t   bufferLen_;
};

} // namespace otpkit
