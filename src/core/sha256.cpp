#include "otpkit/sha256.hpp"

#include <algorithm>
#include <cstring>

namespace otpkit {

namespace {

const std::uint32_t K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

inline std::uint32_t rotr32(std::uint32_t x, int n) noexcept {
    return (x >> n) | (x << (32 - n));
}

} // namespace

SHA256::SHA256() {
    h_[0]=0x6a09e667u; h_[1]=0xbb67ae85u; h_[2]=0x3c6ef372u; h_[3]=0xa54ff53au;
    h_[4]=0x510e527fu; h_[5]=0x9b05688cu; h_[6]=0x1f83d9abu; h_[7]=0x5be0cd19u;
    totalLen_ = 0; bufferLen_ = 0;
}

void SHA256::processBlock(const std::uint8_t* block) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (std::uint32_t(block[i*4])     << 24) |
               (std::uint32_t(block[i*4 + 1]) << 16) |
               (std::uint32_t(block[i*4 + 2]) << 8)  |
                std::uint32_t(block[i*4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = rotr32(w[i-15],7) ^ rotr32(w[i-15],18) ^ (w[i-15] >> 3);
        const std::uint32_t s1 = rotr32(w[i-2],17) ^ rotr32(w[i-2],19)  ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    std::uint32_t a=h_[0],b=h_[1],c=h_[2],d=h_[3],e=h_[4],f=h_[5],g=h_[6],h=h_[7];
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t S1 = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
        const std::uint32_t ch = (e & f) ^ ((~e) & g);
        const std::uint32_t t1 = h + S1 + ch + K256[i] + w[i];
        const std::uint32_t S0 = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
        const std::uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = S0 + mj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h_[0]+=a; h_[1]+=b; h_[2]+=c; h_[3]+=d;
    h_[4]+=e; h_[5]+=f; h_[6]+=g; h_[7]+=h;
}

void SHA256::update(const std::uint8_t* data, std::size_t len) {
    totalLen_ += len;
    while (len > 0) {
        const std::size_t copy = std::min(len, BLOCK_SIZE - bufferLen_);
        std::memcpy(buffer_ + bufferLen_, data, copy);
        bufferLen_ += copy; data += copy; len -= copy;
        if (bufferLen_ == BLOCK_SIZE) {
            processBlock(buffer_);
            bufferLen_ = 0;
        }
    }
}

std::array<std::uint8_t, SHA256::DIGEST_SIZE> SHA256::finalize() {
    std::uint64_t bitLen = totalLen_ * 8;
    const std::uint8_t pad = 0x80;
    update(&pad, 1);
    const std::uint8_t zero = 0x00;
    while (bufferLen_ != 56) update(&zero, 1);

    std::uint8_t lenBytes[8];
    for (int i = 7; i >= 0; --i) {
        lenBytes[i] = static_cast<std::uint8_t>(bitLen & 0xFF);
        bitLen >>= 8;
    }
    update(lenBytes, 8);

    std::array<std::uint8_t, DIGEST_SIZE> out{};
    for (int i = 0; i < 8; ++i) {
        out[i*4 + 0] = static_cast<std::uint8_t>(h_[i] >> 24);
        out[i*4 + 1] = static_cast<std::uint8_t>(h_[i] >> 16);
        out[i*4 + 2] = static_cast<std::uint8_t>(h_[i] >> 8);
        out[i*4 + 3] = static_cast<std::uint8_t>(h_[i]);
    }
    return out;
}

std::array<std::uint8_t, SHA256::DIGEST_SIZE>
SHA256::hash(const std::vector<std::uint8_t>& data) {
    SHA256 s; s.update(data.data(), data.size()); return s.finalize();
}

} // namespace otpkit
