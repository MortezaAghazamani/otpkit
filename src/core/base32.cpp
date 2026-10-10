#include "otpkit/base32.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>

namespace otpkit {

namespace {
const char* ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
}

std::string base32Encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve((data.size() * 8 + 4) / 5);

    int buffer = 0, bitsLeft = 0;
    for (std::uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bitsLeft += 8;
        while (bitsLeft >= 5) {
            bitsLeft -= 5;
            out.push_back(ALPHABET[(buffer >> bitsLeft) & 0x1F]);
        }
    }
    if (bitsLeft > 0) {
        buffer <<= (5 - bitsLeft);
        out.push_back(ALPHABET[buffer & 0x1F]);
    }
    return out;
}

std::vector<std::uint8_t> base32Decode(const std::string& s) {
    std::vector<std::uint8_t> out;
    out.reserve(s.size() * 5 / 8);

    int buffer = 0, bitsLeft = 0;
    for (char ch : s) {
        if (ch == '=' || std::isspace(static_cast<unsigned char>(ch))) continue;
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

        const char* p = std::strchr(ALPHABET, ch);
        if (!p) throw std::invalid_argument("Invalid Base32 character");
        const int val = static_cast<int>(p - ALPHABET);

        buffer = (buffer << 5) | val;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bitsLeft) & 0xFF));
        }
    }
    return out;
}

std::string normalizeBase32(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '=' || std::isspace(static_cast<unsigned char>(ch)) || ch == '-') {
            continue;
        }
        const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        if (!std::strchr(ALPHABET, up)) {
            throw std::invalid_argument(
                std::string("Invalid Base32 character: ") + ch);
        }
        out.push_back(up);
    }
    return out;
}

} // namespace otpkit
