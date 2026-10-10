#include "otpkit/sha1.hpp"

#include <algorithm>
#include <cstring>

namespace otpkit {

namespace {
inline std::uint32_t rotl32(std::uint32_t x, int n) noexcept {
    return (x << n) | (x >> (32 - n));
}
} // namespace

SHA1::SHA1() {
    h_[0] = 0x67452301u;
    h_[1] = 0xEFCDAB89u;
    h_[2] = 0x98BADCFEu;
    h_[3] = 0x10325476u;
    h_[4] = 0xC3D2E1F0u;
    totalLen_  = 0;
    bufferLen_ = 0;
}

void SHA1::processBlock(const std::uint8_t* block) {
    std::uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (std::uint32_t(block[i * 4])     << 24) |
               (std::uint32_t(block[i * 4 + 1]) << 16) |
               (std::uint32_t(block[i * 4 + 2]) << 8)  |
                std::uint32_t(block[i * 4 + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    std::uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3], e = h_[4];
    for (int i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20)      { f = (b & c) | ((~b) & d);         k = 0x5A827999u; }
        else if (i < 40) { f = b ^ c ^ d;                    k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d);  k = 0x8F1BBCDCu; }
        else             { f = b ^ c ^ d;                    k = 0xCA62C1D6u; }

        const std::uint32_t tmp = rotl32(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl32(b, 30); b = a; a = tmp;
    }
    h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d; h_[4] += e;
}

void SHA1::update(const std::uint8_t* data, std::size_t len) {
    totalLen_ += len;
    while (len > 0) {
        const std::size_t copy = std::min(len, BLOCK_SIZE - bufferLen_);
        std::memcpy(buffer_ + bufferLen_, data, copy);
        bufferLen_ += copy;
        data += copy;
        len  -= copy;
        if (bufferLen_ == BLOCK_SIZE) {
            processBlock(buffer_);
            bufferLen_ = 0;
        }
    }
}

void SHA1::update(const std::vector<std::uint8_t>& data) {
    update(data.data(), data.size());
}

std::array<std::uint8_t, SHA1::DIGEST_SIZE> SHA1::finalize() {
    std::uint64_t bitLen = totalLen_ * 8;

    // Padding: append 0x80, then zeros, then 64-bit big-endian length.
    const std::uint8_t pad = 0x80;
    update(&pad, 1);

    const std::uint8_t zero = 0x00;
    while (bufferLen_ != 56) {
        update(&zero, 1);
    }

    std::uint8_t lenBytes[8];
    for (int i = 7; i >= 0; --i) {
        lenBytes[i] = static_cast<std::uint8_t>(bitLen & 0xFF);
        bitLen >>= 8;
    }
    update(lenBytes, 8);

    std::array<std::uint8_t, DIGEST_SIZE> out{};
    for (int i = 0; i < 5; ++i) {
        out[i * 4 + 0] = static_cast<std::uint8_t>(h_[i] >> 24);
        out[i * 4 + 1] = static_cast<std::uint8_t>(h_[i] >> 16);
        out[i * 4 + 2] = static_cast<std::uint8_t>(h_[i] >> 8);
        out[i * 4 + 3] = static_cast<std::uint8_t>(h_[i]);
    }
    return out;
}

std::array<std::uint8_t, SHA1::DIGEST_SIZE>
SHA1::hash(const std::vector<std::uint8_t>& data) {
    SHA1 s;
    s.update(data);
    return s.finalize();
}

} // namespace otpkit
