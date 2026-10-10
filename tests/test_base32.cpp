#include <catch2/catch_test_macros.hpp>

#include "otpkit/base32.hpp"

#include <string>
#include <vector>

TEST_CASE("Base32 RFC 4648 vectors", "[base32]") {
    struct V { const char* in; const char* out; };
    const V v[] = {
        { "f",       "MY"       },
        { "fo",      "MZXQ"     },
        { "foo",     "MZXW6"    },
        { "foob",    "MZXW6YQ"  },
        { "fooba",   "MZXW6YTB" },
        { "foobar",  "MZXW6YTBOI" }
    };
    for (const auto& t : v) {
        const std::vector<std::uint8_t> data(t.in, t.in + std::strlen(t.in));
        REQUIRE(otpkit::base32Encode(data) == t.out);
        REQUIRE(otpkit::base32Decode(t.out) == data);
    }
}

TEST_CASE("Base32 roundtrip random bytes", "[base32]") {
    std::vector<std::uint8_t> data;
    for (int i = 0; i < 20; ++i) data.push_back(static_cast<std::uint8_t>(i * 7 + 1));
    const auto enc = otpkit::base32Encode(data);
    const auto dec = otpkit::base32Decode(enc);
    REQUIRE(dec == data);
}
