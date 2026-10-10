#include "platform/random.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace otpkit::platform {

std::vector<std::uint8_t> secureRandom(std::size_t bytes) {
    std::vector<std::uint8_t> buf(bytes);
    const NTSTATUS st = BCryptGenRandom(
        nullptr,
        buf.data(),
        static_cast<ULONG>(bytes),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (st != 0) throw std::runtime_error("BCryptGenRandom failed");
    return buf;
}

} // namespace otpkit::platform
