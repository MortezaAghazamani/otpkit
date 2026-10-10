/**
 * @file totp.hpp
 * @brief Time-based One-Time Password (RFC 6238) — a thin wrapper over HOTP.
 */
#pragma once

#include "otpkit/config.hpp"
#include "otpkit/hotp.hpp"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace otpkit {

/**
 * @brief TOTP generator/verifier (RFC 6238).
 *
 * Internally uses HOTP with `counter = floor(unixTime / period)`.
 * Compatible with Google Authenticator, Authy, Microsoft Authenticator, etc.
 */
class Totp {
public:
    /// Fresh random secret.
    explicit Totp(Config cfg, std::size_t secretBytes = 20);

    /// Existing Base32 secret.
    Totp(Config cfg, const std::string& base32Secret);

    // ---- Code generation ----

    std::string generateCode() const;
    std::string generateCode(std::uint64_t unixTime) const;

    // ---- Verification ----

    /**
     * @brief Verify a user code.
     * @param userCode Code to verify.
     * @param window   Number of periods tolerated on each side (default 1
     *                 = ±30 s with the standard 30-second period).
     */
    bool verify(const std::string& userCode, int window = 1) const;

    /// Seconds until the current code expires.
    int secondsRemaining() const;

    // ---- Accessors ----

    const std::string& secretBase32() const noexcept { return hotp_.secretBase32(); }
    const Config&      config()       const noexcept { return hotp_.config(); }

    // ---- otpauth URI ----

    std::string otpauthUri(const std::string& issuer,
                           const std::string& accountName) const;

    // ---- Persistence ----

    /// Serialize to a byte buffer (encrypt hook applied).
    std::vector<std::uint8_t> save() const;

    /// Serialize and write to a file.
    void save(const std::string& path) const;

    /// Parse from a byte buffer (decrypt hook applied).
    static Totp load(const std::vector<std::uint8_t>& data);

    /// Read a file and parse it.
    static Totp load(const std::string& path);

protected:
    /**
     * @brief Protected constructor for subclasses.
     *
     * The trailing `int` is an unused tag to disambiguate from the
     * public 2-argument constructor. Subclasses use this to rebuild
     * an instance after decrypting persisted data.
     *
     * @param cfg          Algorithm configuration.
     * @param base32Secret Base32-encoded secret.
     * @param tag          Disambiguation tag (unused).
     */
    Totp(Config cfg, const std::string& base32Secret, int tag);

    /// Hook called by save() to transform serialized bytes. Default: identity.
    virtual std::vector<std::uint8_t>
    encrypt(const std::vector<std::uint8_t>& plain) const;

    /// Hook called by load() to reverse @ref encrypt. Default: identity.
    virtual std::vector<std::uint8_t>
    decrypt(const std::vector<std::uint8_t>& cipher) const;

    /// Serialize the current state to a JSON string (no encrypt hook).
    std::string serializeToJson() const;

    /// Static JSON parser used by @ref load and by subclass loaders.
    static std::tuple<Config, std::string> parseJson(const std::string& json);

private:
    Hotp hotp_;
    int  period_;
};

} // namespace otpkit
