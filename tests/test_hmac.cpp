#include <catch2/catch_test_macros.hpp>

#include "otpkit/hmac.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> bytes(const std::string& s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

std::string hex(const std::vector<std::uint8_t>& v) {
    static const char* d = "0123456789abcdef";
    std::string out;
    out.reserve(v.size() * 2);
    for (auto b : v) { out.push_back(d[b >> 4]); out.push_back(d[b & 0xF]); }
    return out;
}

} // namespace

TEST_CASE("HMAC-SHA1 RFC 2202 test case 1", "[hmac]") {
    std::vector<std::uint8_t> key(20, 0x0b);
    const auto mac = otpkit::hmac(otpkit::Algorithm::SHA1, key, bytes("Hi There"));
    REQUIRE(hex(mac) == "b617318655057264e28bc0b6fb378c8ef146be00");
}

TEST_CASE("HMAC-SHA256 RFC 4231 test case 1", "[hmac]") {
    std::vector<std::uint8_t> key(20, 0x0b);
    const auto mac = otpkit::hmac(otpkit::Algorithm::SHA256, key, bytes("Hi There"));
    REQUIRE(hex(mac) ==
            "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}
