/**
 * @file sha512.hpp
 * @brief Self-contained SHA-512 implementation (FIPS 180-4).
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace otpkit {

/**
 * @brief Incremental SHA-512 hasher.
 */
class SHA512 {
public:
    static constexpr std::size_t DIGEST_SIZE = 64;  ///< Output size in bytes.
    static constexpr std::size_t BLOCK_SIZE  = 128; ///< Internal block size.

    SHA512();

    void update(const std::uint8_t* data, std::size_t len);

    std::array<std::uint8_t, DIGEST_SIZE> finalize();

    static std::array<std::uint8_t, DIGEST_SIZE>
    hash(const std::vector<std::uint8_t>& data);

private:
    void processBlock(const std::uint8_t* block);

    std::uint64_t h_[8];
    std::uint64_t totalLen_;
    std::uint8_t  buffer_[BLOCK_SIZE];
    std::size_t   bufferLen_;
};

} // namespace otpkit
