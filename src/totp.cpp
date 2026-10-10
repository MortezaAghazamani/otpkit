#include "otpkit/totp.hpp"

#include "otpkit/base32.hpp"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace otpkit {

namespace {

std::uint64_t nowUnix() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string urlEncode(const std::string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            o << c;
        } else {
            o << '%' << std::uppercase << std::hex
              << std::setw(2) << std::setfill('0') << int(c);
        }
    }
    return o.str();
}

// --- small JSON helpers (flat schema only) ---

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

Algorithm algorithmFromName(const std::string& s) {
    if (s == "SHA256") return Algorithm::SHA256;
    if (s == "SHA512") return Algorithm::SHA512;
    return Algorithm::SHA1;
}

} // namespace

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

Totp::Totp(Config cfg, std::size_t secretBytes)
    : hotp_(cfg, 0, secretBytes),
      period_(cfg.period)
{}

Totp::Totp(Config cfg, const std::string& base32Secret)
    : hotp_(cfg, base32Secret, 0),
      period_(cfg.period)
{}

Totp::Totp(Config cfg, const std::string& base32Secret, int /*tag*/)
    : hotp_(cfg, base32Secret, 0),
      period_(cfg.period)
{}

// ---------------------------------------------------------------------------
// Code generation
// ---------------------------------------------------------------------------

std::string Totp::generateCode() const {
    return generateCode(nowUnix());
}

std::string Totp::generateCode(std::uint64_t unixTime) const {
    const std::uint64_t counter = unixTime / static_cast<std::uint64_t>(period_);
    return hotp_.generateCode(counter);
}

// ---------------------------------------------------------------------------
// Verification
// ---------------------------------------------------------------------------

bool Totp::verify(const std::string& userCode, int window) const {
    if (static_cast<int>(userCode.size()) != config().digits) return false;
    for (char c : userCode) {
        if (c < '0' || c > '9') return false;
    }

    const std::uint64_t base = nowUnix() / static_cast<std::uint64_t>(period_);
    for (int i = -window; i <= window; ++i) {
        const long long c = static_cast<long long>(base) + i;
        if (c < 0) continue;
        if (hotp_.generateCode(static_cast<std::uint64_t>(c)) == userCode) {
            return true;
        }
    }
    return false;
}

int Totp::secondsRemaining() const {
    const std::uint64_t now = nowUnix();
    const std::uint64_t rem = static_cast<std::uint64_t>(period_) - (now % period_);
    return static_cast<int>(rem);
}

// ---------------------------------------------------------------------------
// otpauth URI
// ---------------------------------------------------------------------------

std::string Totp::otpauthUri(const std::string& issuer,
                             const std::string& accountName) const
{
    std::ostringstream uri;
    uri << "otpauth://totp/"
        << urlEncode(issuer) << ":" << urlEncode(accountName)
        << "?secret="    << secretBase32()
        << "&issuer="    << urlEncode(issuer)
        << "&algorithm=" << algorithmName(config().algorithm)
        << "&digits="    << config().digits
        << "&period="    << period_;
    return uri.str();
}

// ---------------------------------------------------------------------------
// Persistence hooks (default = identity)
// ---------------------------------------------------------------------------

std::vector<std::uint8_t>
Totp::encrypt(const std::vector<std::uint8_t>& plain) const {
    return plain;
}

std::vector<std::uint8_t>
Totp::decrypt(const std::vector<std::uint8_t>& cipher) const {
    return cipher;
}

// ---------------------------------------------------------------------------
// JSON serialization / parsing
// ---------------------------------------------------------------------------

std::string Totp::serializeToJson() const {
    std::string json;
    json += "{\n";
    json += "  \"type\": \"totp\",\n";
    json += "  \"algorithm\": \"" + std::string(algorithmName(config().algorithm)) + "\",\n";
    json += "  \"digits\": " + std::to_string(config().digits) + ",\n";
    json += "  \"period\": " + std::to_string(period_) + ",\n";
    json += "  \"secret\": \"" + jsonEscape(secretBase32()) + "\"\n";
    json += "}\n";
    return json;
}

std::tuple<Config, std::string>
Totp::parseJson(const std::string& json) {
    Config cfg;
    cfg.algorithm = algorithmFromName(extractString(json, "algorithm"));
    cfg.digits    = static_cast<int>(extractInt(json, "digits", 6));
    cfg.period    = static_cast<int>(extractInt(json, "period", 30));
    const std::string secret = extractString(json, "secret");
    return { cfg, secret };
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

std::vector<std::uint8_t> Totp::save() const {
    const std::string json = serializeToJson();
    std::vector<std::uint8_t> plain(json.begin(), json.end());
    return encrypt(plain);
}

void Totp::save(const std::string& path) const {
    const std::vector<std::uint8_t> bytes = save();
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file for writing: " + path);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

Totp Totp::load(const std::vector<std::uint8_t>& data) {
    // Base load: assume plain storage (identity decrypt).
    Config cfg;
    Totp tmp(cfg, std::string("AA"));
    const std::vector<std::uint8_t> plain = tmp.decrypt(data);

    const std::string json(plain.begin(), plain.end());
    auto [parsedCfg, secret] = parseJson(json);
    return Totp(parsedCfg, secret);
}

Totp Totp::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(f)),
         std::istreambuf_iterator<char>());
    return load(data);
}

} // namespace otpkit
