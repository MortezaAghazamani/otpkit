/**
 * @file hotp.hpp
 * @brief HMAC-based One-Time Password (RFC 4226).
 */
#pragma once

#include "otpkit/config.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace otpkit {

/**
 * @brief Generate a random Base32-encoded secret.
 *
 * @param bytes Number of random bytes (default 20 = 160 bits, as
 *              recommended by RFC 4226).
 * @return Unpadded Base32 string.
 */
std::string generateSecretBase32(std::size_t bytes = 20);

/**
 * @brief HOTP generator/verifier (RFC 4226).
 *
 * Supports both stateful and stateless use:
 *  - Stateful:  hold a counter, call generateCode()/verify().
 *  - Stateless: call generateCode(counter) with an external counter.
 */
class Hotp {
public:
    /**
     * @brief Construct with a freshly generated random secret.
     * @param cfg            Algorithm configuration.
     * @param initialCounter Initial counter value (default 0).
     * @param secretBytes    Secret size in bytes (default 20).
     * @throws std::invalid_argument if secretBytes is out of range.
     */
    explicit Hotp(Config cfg,
                  std::uint64_t initialCounter = 0,
                  std::size_t secretBytes = 20);

    /**
     * @brief Construct from an existing Base32 secret.
     * @param cfg            Algorithm configuration.
     * @param base32Secret   Base32-encoded secret (padded or unpadded).
     * @param initialCounter Initial counter value (default 0).
     * @throws std::invalid_argument on invalid Base32 input.
     */
    Hotp(Config cfg,
         const std::string& base32Secret,
         std::uint64_t initialCounter = 0);

    // ---- Stateless API ----

    /**
     * @brief Generate the code for a specific counter (no state change).
     */
    std::string generateCode(std::uint64_t counter) const;

    // ---- Stateful API ----

    /// Generate the code using the internal counter.
    std::string generateCode() const;

    /// Advance the internal counter by one.
    void incrementCounter();

    /// Return the internal counter.
    std::uint64_t counter() const noexcept;

    /// Set the internal counter.
    void setCounter(std::uint64_t c) noexcept;

    // ---- Verification ----

    /**
     * @brief Verify a user-supplied code.
     *
     * Tries counters `counter_ + 0 .. counter_ + window` (never backward)
     * to allow for a user skipping ahead.
     *
     * @param userCode         Code to verify (must have exactly `digits` digits).
     * @param window           Number of counters ahead to try.
     * @param advanceOnSuccess If true, on success the internal counter is set
     *                         to (matched counter + 1).
     * @return true if the code matches.
     */
    bool verify(const std::string& userCode,
                int  window = 10,
                bool advanceOnSuccess = true);

    // ---- Accessors ----

    const std::string& secretBase32() const noexcept { return secretBase32_; }
    const Config&      config()       const noexcept { return cfg_; }

    // ---- Persistence ----

    /**
     * @brief Serialize the current state to an in-memory byte buffer.
     *
     * The hook @ref encrypt is applied to the serialized JSON.
     */
    std::vector<std::uint8_t> save() const;

    /// Serialize and write to a file (binary mode).
    void save(const std::string& path) const;

    /// Parse from an in-memory byte buffer. Applies @ref decrypt.
    static Hotp load(const std::vector<std::uint8_t>& data);

    /// Read a file and parse it.
    static Hotp load(const std::string& path);

protected:
    /**
     * @brief Hook called by save() to transform serialized bytes.
     *
     * Default implementation returns @p plain unchanged (plain storage).
     * Override to add encryption (e.g., AES-GCM, DPAPI).
     */
    virtual std::vector<std::uint8_t>
    encrypt(const std::vector<std::uint8_t>& plain) const;

    /**
     * @brief Hook called by load() to reverse @ref encrypt.
     *
     * Default implementation returns @p cipher unchanged.
     * Must be overridden symmetrically if @ref encrypt is overridden.
     */
    virtual std::vector<std::uint8_t>
    decrypt(const std::vector<std::uint8_t>& cipher) const;

private:
    std::vector<std::uint8_t> secretRaw_;
    std::string               secretBase32_;
    Config                    cfg_;
    std::uint64_t             counter_;
};

} // namespace otpkit
