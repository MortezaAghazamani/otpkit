#include <catch2/catch_test_macros.hpp>

#include "otpkit/sha1.hpp"
#include "otpkit/sha256.hpp"
#include "otpkit/sha512.hpp"

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

TEST_CASE("SHA1 empty string", "[sha]") {
    const auto h = otpkit::SHA1::hash({});
    REQUIRE(hex(std::vector<std::uint8_t>(h.begin(), h.end())) ==
            "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST_CASE("SHA1 abc", "[sha]") {
    const auto h = otpkit::SHA1::hash(bytes("abc"));
    REQUIRE(hex(std::vector<std::uint8_t>(h.begin(), h.end())) ==
            "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST_CASE("SHA256 abc", "[sha]") {
    const auto h = otpkit::SHA256::hash(bytes("abc"));
    REQUIRE(hex(std::vector<std::uint8_t>(h.begin(), h.end())) ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA512 abc", "[sha]") {
    const auto h = otpkit::SHA512::hash(bytes("abc"));
    REQUIRE(hex(std::vector<std::uint8_t>(h.begin(), h.end())) ==
            "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
            "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}
