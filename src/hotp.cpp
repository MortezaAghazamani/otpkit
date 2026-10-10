#include "otpkit/hotp.hpp"

#include "otpkit/base32.hpp"
#include "otpkit/hmac.hpp"
#include "platform/random.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <string>

namespace otpkit {

// ---------------------------------------------------------------------------
// Helper: serialization
// ---------------------------------------------------------------------------

namespace {

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else out.push_back(c);
    }
    return out;
}

std::string extractString(const std::string& json, const std::string& key) {
    const std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    const auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

long long extractInt(const std::string& json, const std::string& key, long long defVal) {
    const std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return defVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defVal;
    ++pos;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    long long val = 0;
    bool any = false;
    while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
        val = val * 10 + (json[pos] - '0');
        ++pos; any = true;
    }
    return any ? val : defVal;
}

} // namespace

const char* algorithmName(Algorithm algo) noexcept {
    switch (algo) {
        case Algorithm::SHA1:   return "SHA1";
        case Algorithm::SHA256: return "SHA256";
        case Algorithm::SHA512: return "SHA512";
    }
    return "SHA1";
}

namespace {

Algorithm algorithmFromName(const std::string& s) {
    if (s == "SHA256") return Algorithm::SHA256;
    if (s == "SHA512") return Algorithm::SHA512;
    return Algorithm::SHA1;
}

} // namespace

// ---------------------------------------------------------------------------
// Free helpers
// ---------------------------------------------------------------------------

std::string generateSecretBase32(std::size_t bytes) {
    return base32Encode(platform::secureRandom(bytes));
}

// ---------------------------------------------------------------------------
// Hotp
// ---------------------------------------------------------------------------

Hotp::Hotp(Config cfg, std::uint64_t initialCounter, std::size_t secretBytes)
    : cfg_(std::move(cfg)), counter_(initialCounter)
{
    if (secretBytes < 10 || secretBytes > 32) {
        throw std::invalid_argument("secretBytes must be between 10 and 32");
    }
    secretRaw_ = platform::secureRandom(secretBytes);
    secretBase32_ = base32Encode(secretRaw_);
}

Hotp::Hotp(Config cfg, const std::string& base32Secret, std::uint64_t initialCounter)
    : secretBase32_(base32Secret),
      cfg_(std::move(cfg)),
      counter_(initialCounter)
{
    secretRaw_ = base32Decode(base32Secret);
    if (secretRaw_.empty()) {
        throw std::invalid_argument("Invalid or empty Base32 secret");
    }
}

std::string Hotp::generateCode(std::uint64_t counter) const {
    // Encode the counter as 8 big-endian bytes.
    std::vector<std::uint8_t> msg(8);
    for (int i = 7; i >= 0; --i) {
        msg[i] = static_cast<std::uint8_t>(counter & 0xFF);
        counter >>= 8;
    }

    const std::vector<std::uint8_t> mac = hmac(cfg_.algorithm, secretRaw_, msg);

    // Dynamic truncation per RFC 4226 section 5.3.
    const int offset = mac.back() & 0x0F;
    const std::uint32_t binary =
        (std::uint32_t(mac[offset]     & 0x7F) << 24) |
        (std::uint32_t(mac[offset + 1] & 0xFF) << 16) |
        (std::uint32_t(mac[offset + 2] & 0xFF) << 8)  |
         std::uint32_t(mac[offset + 3] & 0xFF);

    std::uint32_t mod = 1;
    for (int i = 0; i < cfg_.digits; ++i) mod *= 10;
    std::uint32_t code = binary % mod;

    std::string out(cfg_.digits, '0');
    for (int i = cfg_.digits - 1; i >= 0; --i) {
        out[i] = static_cast<char>('0' + (code % 10));
        code /= 10;
    }
    return out;
}

std::string Hotp::generateCode() const {
    return generateCode(counter_);
}

void Hotp::incrementCounter() { ++counter_; }

std::uint64_t Hotp::counter() const noexcept { return counter_; }

void Hotp::setCounter(std::uint64_t c) noexcept { counter_ = c; }

bool Hotp::verify(const std::string& userCode, int window, bool advanceOnSuccess) {
    if (static_cast<int>(userCode.size()) != cfg_.digits) return false;
    for (char c : userCode) {
        if (c < '0' || c > '9') return false;
    }

    for (int i = 0; i <= window; ++i) {
        const std::uint64_t c = counter_ + static_cast<std::uint64_t>(i);
        if (generateCode(c) == userCode) {
            if (advanceOnSuccess) counter_ = c + 1;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Persistence hooks (default = identity / plain storage)
// ---------------------------------------------------------------------------

std::vector<std::uint8_t>
Hotp::encrypt(const std::vector<std::uint8_t>& plain) const {
    return plain;
}

std::vector<std::uint8_t>
Hotp::decrypt(const std::vector<std::uint8_t>& cipher) const {
    return cipher;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

std::vector<std::uint8_t> Hotp::save() const {
    std::string json;
    json += "{\n";
    json += "  \"type\": \"hotp\",\n";
    json += "  \"algorithm\": \"" + std::string(algorithmName(cfg_.algorithm)) + "\",\n";
    json += "  \"digits\": " + std::to_string(cfg_.digits) + ",\n";
    json += "  \"secret\": \"" + jsonEscape(secretBase32_) + "\",\n";
    json += "  \"counter\": " + std::to_string(counter_) + "\n";
    json += "}\n";

    std::vector<std::uint8_t> plain(json.begin(), json.end());
    return encrypt(plain);
}

void Hotp::save(const std::string& path) const {
    const std::vector<std::uint8_t> bytes = save();
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file for writing: " + path);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

Hotp Hotp::load(const std::vector<std::uint8_t>& data) {
    const std::vector<std::uint8_t> plain = [&] {
        // decrypt must be a virtual on a live instance. Since load is static,
        // we temporarily create an identity instance to call decrypt.
        // Subclasses overriding decrypt are expected to also expose their
        // own load(), or use the same class for save and load.
        // For the base class this is identity.
        Config cfg;
        Hotp tmp(cfg, std::string("AA"), 0);
        return tmp.decrypt(data);
    }();

    const std::string json(plain.begin(), plain.end());

    Config cfg;
    cfg.algorithm = algorithmFromName(extractString(json, "algorithm"));
    cfg.digits    = static_cast<int>(extractInt(json, "digits", 6));

    const std::string secret  = extractString(json, "secret");
    const auto        counter = static_cast<std::uint64_t>(extractInt(json, "counter", 0));

    return Hotp(cfg, secret, counter);
}

Hotp Hotp::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(f)),
         std::istreambuf_iterator<char>());
    return load(data);
}

} // namespace otpkit
