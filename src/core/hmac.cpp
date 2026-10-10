#include "otpkit/hmac.hpp"

#include "otpkit/sha1.hpp"
#include "otpkit/sha256.hpp"
#include "otpkit/sha512.hpp"

namespace otpkit {

namespace detail {

template <typename HashT>
std::vector<std::uint8_t>
hmacImpl(const std::vector<std::uint8_t>& key,
         const std::vector<std::uint8_t>& message)
{
    constexpr std::size_t BLOCK = HashT::BLOCK_SIZE;

    // Keys longer than the block size are hashed first.
    std::vector<std::uint8_t> k = key;
    if (k.size() > BLOCK) {
        const auto h = HashT::hash(k);
        k.assign(h.begin(), h.end());
    }
    k.resize(BLOCK, 0x00);

    std::vector<std::uint8_t> oKeyPad(BLOCK), iKeyPad(BLOCK);
    for (std::size_t i = 0; i < BLOCK; ++i) {
        oKeyPad[i] = static_cast<std::uint8_t>(k[i] ^ 0x5c);
        iKeyPad[i] = static_cast<std::uint8_t>(k[i] ^ 0x36);
    }

    // inner = H(iKeyPad || message)
    HashT h1;
    h1.update(iKeyPad.data(), iKeyPad.size());
    h1.update(message.data(), message.size());
    const auto inner = h1.finalize();

    // outer = H(oKeyPad || inner)
    HashT h2;
    h2.update(oKeyPad.data(), oKeyPad.size());
    h2.update(inner.data(), inner.size());
    const auto outer = h2.finalize();

    return std::vector<std::uint8_t>(outer.begin(), outer.end());
}

} // namespace detail

std::vector<std::uint8_t> hmac(Algorithm algo,
                               const std::vector<std::uint8_t>& key,
                               const std::vector<std::uint8_t>& message)
{
    switch (algo) {
        case Algorithm::SHA1:   return detail::hmacImpl<SHA1>(key, message);
        case Algorithm::SHA256: return detail::hmacImpl<SHA256>(key, message);
        case Algorithm::SHA512: return detail::hmacImpl<SHA512>(key, message);
    }
    return {};
}

} // namespace otpkit
