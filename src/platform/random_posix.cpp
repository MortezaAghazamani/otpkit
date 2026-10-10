#include "platform/random.hpp"

#include <fstream>
#include <stdexcept>

namespace otpkit::platform {

std::vector<std::uint8_t> secureRandom(std::size_t bytes) {
    std::vector<std::uint8_t> buf(bytes);
    std::ifstream f("/dev/urandom", std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open /dev/urandom");
    f.read(reinterpret_cast<char*>(buf.data()),
           static_cast<std::streamsize>(buf.size()));
    if (!f) throw std::runtime_error("Read from /dev/urandom failed");
    return buf;
}

} // namespace otpkit::platform
