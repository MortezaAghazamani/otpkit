#include <catch2/catch_test_macros.hpp>
#include "otpkit/base32.hpp"

#include "otpkit/totp.hpp"

#include <string>

TEST_CASE("RFC 6238 TOTP SHA1 vectors", "[totp]") {
    // The RFC uses an ASCII secret, 20 bytes.
    const std::string ascii = "12345678901234567890";
    const std::vector<std::uint8_t> raw(ascii.begin(), ascii.end());
    const std::string base32 = otpkit::base32Encode(raw);

    otpkit::Config cfg;
    cfg.algorithm = otpkit::Algorithm::SHA1;
    cfg.digits    = 8;
    cfg.period    = 30;

    otpkit::Totp totp(cfg, base32);

    REQUIRE(totp.generateCode(59)         == "94287082");
    REQUIRE(totp.generateCode(1111111109) == "07081804");
    REQUIRE(totp.generateCode(1111111111) == "14050471");
    REQUIRE(totp.generateCode(1234567890) == "89005924");
    REQUIRE(totp.generateCode(2000000000) == "69279037");
    REQUIRE(totp.generateCode(20000000000ULL) == "65353130");
}

TEST_CASE("TOTP 6-digit Google Authenticator format", "[totp]") {
    otpkit::Config cfg;   // default: SHA1, 6 digits, 30 seconds
    otpkit::Totp totp(cfg, 20);

    const std::string code = totp.generateCode();
    REQUIRE(code.size() == 6);
    for (char c : code) REQUIRE(c >= '0');
    for (char c : code) REQUIRE(c <= '9');

    REQUIRE(totp.verify(code, 1));
}
