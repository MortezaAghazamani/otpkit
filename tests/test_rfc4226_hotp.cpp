#include <catch2/catch_test_macros.hpp>

#include "otpkit/base32.hpp"
#include "otpkit/hotp.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

    /// RFC 4226 uses the ASCII secret "12345678901234567890".
    std::string rfcSecretBase32() {
        const std::string ascii = "12345678901234567890";
        const std::vector<std::uint8_t> raw(ascii.begin(), ascii.end());
        return otpkit::base32Encode(raw);
    }

} // namespace

TEST_CASE("RFC 4226 HOTP test vectors", "[hotp]") {
    const std::string secretBase32 = rfcSecretBase32();

    otpkit::Config cfg;   // SHA1, 6 digits
    otpkit::Hotp   hotp(cfg, secretBase32, 0);

    const char* expected[] = {
        "755224", "287082", "359152", "969429", "338314",
        "254676", "287922", "162583", "399871", "520489"
    };

    for (int i = 0; i < 10; ++i) {
        REQUIRE(hotp.generateCode(static_cast<std::uint64_t>(i)) == expected[i]);
    }
}

TEST_CASE("HOTP stateful verify advances counter", "[hotp]") {
    otpkit::Config cfg;
    otpkit::Hotp   hotp(cfg, /*initialCounter=*/0, /*secretBytes=*/20);

    const std::string code = hotp.generateCode();
    REQUIRE(hotp.verify(code, 0, true));
    REQUIRE(hotp.counter() == 1);
}