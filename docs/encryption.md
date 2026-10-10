# Encrypting otpkit secrets

By default, `otpkit` stores secrets as **plain JSON**. Anyone with read
access to the file can extract the secret and generate valid OTP codes.

This document shows two production-ready ways to protect the secret at
rest:

1. **Windows DPAPI** — no password, tied to the Windows user account.
2. **Password-based encryption** with libsodium — portable across
   platforms, requires the user to remember a password.

Both are implemented as subclasses of `otpkit::Totp` (or `otpkit::Hotp`).
They override the protected `encrypt` / `decrypt` hooks and provide their
own static `load` methods.

---

## 1. Windows DPAPI

DPAPI encrypts data using credentials tied to the current Windows user.
No password is needed; the OS manages the master key. This is ideal for
desktop applications where the secret only needs to be readable by the
same user on the same machine.

### `dpapi_totp.hpp`

```cpp
#pragma once

#include "otpkit/totp.hpp"

#include <string>

namespace otpkit::examples {

class DpapiTotp : public otpkit::Totp {
public:
    DpapiTotp(otpkit::Config cfg, std::size_t secretBytes = 20);
    DpapiTotp(otpkit::Config cfg, const std::string& base32Secret);

    /// Load a DPAPI-protected file.
    static DpapiTotp load(const std::string& path);

protected:
    std::vector<std::uint8_t>
    encrypt(const std::vector<std::uint8_t>& plain) const override;

    std::vector<std::uint8_t>
    decrypt(const std::vector<std::uint8_t>& cipher) const override;

private:
    DpapiTotp(otpkit::Config cfg, const std::string& base32Secret, int);
};

} // namespace otpkit::examples
```

### `dpapi_totp.cpp`

```cpp
#include "dpapi_totp.hpp"

#include <windows.h>
#include <wincrypt.h>

#include <fstream>
#include <stdexcept>

#pragma comment(lib, "crypt32.lib")

namespace otpkit::examples {

DpapiTotp::DpapiTotp(otpkit::Config cfg, std::size_t secretBytes)
    : Totp(cfg, secretBytes) {}

DpapiTotp::DpapiTotp(otpkit::Config cfg, const std::string& base32Secret)
    : Totp(cfg, base32Secret) {}

DpapiTotp::DpapiTotp(otpkit::Config cfg, const std::string& base32Secret, int)
    : Totp(cfg, base32Secret, 0) {}

std::vector<std::uint8_t>
DpapiTotp::encrypt(const std::vector<std::uint8_t>& plain) const {
    DATA_BLOB in{
        static_cast<DWORD>(plain.size()),
        const_cast<BYTE*>(plain.data())
    };
    DATA_BLOB out{};

    if (!CryptProtectData(&in, L"otpkit", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        throw std::runtime_error("CryptProtectData failed");
    }

    std::vector<std::uint8_t> result(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return result;
}

std::vector<std::uint8_t>
DpapiTotp::decrypt(const std::vector<std::uint8_t>& cipher) const {
    DATA_BLOB in{
        static_cast<DWORD>(cipher.size()),
        const_cast<BYTE*>(cipher.data())
    };
    DATA_BLOB out{};

    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        throw std::runtime_error("CryptUnprotectData failed");
    }

    std::vector<std::uint8_t> result(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return result;
}

DpapiTotp DpapiTotp::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(f)),
         std::istreambuf_iterator<char>());

    DpapiTotp tmp(otpkit::Config{});
    const auto plain = tmp.decrypt(data);
    const std::string json(plain.begin(), plain.end());
    auto [cfg, secret] = parseJson(json);
    return DpapiTotp(cfg, secret, 0);
}

} // namespace otpkit::examples
```

### Usage

```cpp
#include "dpapi_totp.hpp"

int main() {
    otpkit::Config cfg;
    otpkit::examples::DpapiTotp auth(cfg, 20);

    auth.save("myauth.dpapi");

    auto loaded = otpkit::examples::DpapiTotp::load("myauth.dpapi");
    std::cout << "Same code: "
              << (loaded.generateCode() == auth.generateCode() ? "yes" : "no")
              << "\n";
}
```

### CMake

```cmake
add_executable(my_app main.cpp dpapi_totp.cpp)
target_link_libraries(my_app PRIVATE otpkit::core crypt32)
```

### Notes

- The file is readable **only** by the same user on the same machine.
- Moving the file to another machine or user will make it unreadable.
- No password prompt is needed — this is a convenience and a limitation.
- Uses `CRYPTPROTECT_UI_FORBIDDEN` so no dialog appears.

---

## 2. Password-based encryption with libsodium

For portable, password-protected storage, use libsodium:

- **Argon2id** for key derivation.
- **XChaCha20-Poly1305** for authenticated encryption.

### Dependencies

Add `libsodium` to your project. With vcpkg:

```json
"dependencies": [ "libsodium" ]
```

### `encrypted_totp.hpp`

```cpp
#pragma once

#include "otpkit/totp.hpp"

#include <string>

namespace otpkit::examples {

class EncryptedTotp : public otpkit::Totp {
public:
    EncryptedTotp(const std::string& password,
                  otpkit::Config cfg,
                  std::size_t secretBytes = 20);

    EncryptedTotp(const std::string& password,
                  otpkit::Config cfg,
                  const std::string& base32Secret);

    static EncryptedTotp load(const std::string& password,
                              const std::string& path);

    static EncryptedTotp load(const std::string& password,
                              const std::vector<std::uint8_t>& data);

protected:
    std::vector<std::uint8_t>
    encrypt(const std::vector<std::uint8_t>& plain) const override;

    std::vector<std::uint8_t>
    decrypt(const std::vector<std::uint8_t>& cipher) const override;

private:
    EncryptedTotp(const std::string& password,
                  otpkit::Config cfg,
                  const std::string& base32Secret,
                  int);

    std::string password_;
};

} // namespace otpkit::examples
```

### `encrypted_totp.cpp`

```cpp
#include "encrypted_totp.hpp"

#include <sodium.h>

#include <fstream>
#include <stdexcept>
#include <tuple>

namespace otpkit::examples {

namespace {

constexpr std::size_t SALT_LEN  = 16;
constexpr std::size_t NONCE_LEN = 24;

std::vector<std::uint8_t>
deriveKey(const std::string& password,
          const std::uint8_t* salt, std::size_t saltLen)
{
    std::vector<std::uint8_t> key(
        crypto_aead_xchacha20poly1305_ietf_KEYBYTES);

    if (crypto_pwhash(
            key.data(), key.size(),
            password.data(), password.size(),
            salt,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("crypto_pwhash failed");
    }
    return key;
}

} // namespace

EncryptedTotp::EncryptedTotp(const std::string& password,
                             otpkit::Config cfg,
                             std::size_t secretBytes)
    : Totp(cfg, secretBytes), password_(password)
{
    if (sodium_init() < 0) throw std::runtime_error("sodium_init failed");
}

EncryptedTotp::EncryptedTotp(const std::string& password,
                             otpkit::Config cfg,
                             const std::string& base32Secret)
    : Totp(cfg, base32Secret), password_(password)
{
    if (sodium_init() < 0) throw std::runtime_error("sodium_init failed");
}

EncryptedTotp::EncryptedTotp(const std::string& password,
                             otpkit::Config cfg,
                             const std::string& base32Secret,
                             int)
    : Totp(cfg, base32Secret, 0), password_(password)
{}

std::vector<std::uint8_t>
EncryptedTotp::encrypt(const std::vector<std::uint8_t>& plain) const {
    std::vector<std::uint8_t> salt(SALT_LEN);
    std::vector<std::uint8_t> nonce(NONCE_LEN);
    randombytes_buf(salt.data(), salt.size());
    randombytes_buf(nonce.data(), nonce.size());

    const auto key = deriveKey(password_, salt.data(), salt.size());

    std::vector<std::uint8_t> cipher(
        plain.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long cipherLen = 0;

    crypto_aead_xchacha20poly1305_ietf_encrypt(
        cipher.data(), &cipherLen,
        plain.data(), plain.size(),
        nullptr, 0,
        nullptr,
        nonce.data(),
        key.data());

    cipher.resize(cipherLen);

    std::vector<std::uint8_t> out;
    out.reserve(salt.size() + nonce.size() + cipher.size());
    out.insert(out.end(), salt.begin(), salt.end());
    out.insert(out.end(), nonce.begin(), nonce.end());
    out.insert(out.end(), cipher.begin(), cipher.end());
    return out;
}

std::vector<std::uint8_t>
EncryptedTotp::decrypt(const std::vector<std::uint8_t>& cipher) const {
    if (cipher.size() < SALT_LEN + NONCE_LEN +
                        crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        throw std::runtime_error("Encrypted payload too short");
    }

    const std::uint8_t* salt  = cipher.data();
    const std::uint8_t* nonce = cipher.data() + SALT_LEN;
    const std::uint8_t* ct    = cipher.data() + SALT_LEN + NONCE_LEN;
    const std::size_t   ctLen = cipher.size() - SALT_LEN - NONCE_LEN;

    const auto key = deriveKey(password_, salt, SALT_LEN);

    std::vector<std::uint8_t> plain(ctLen);
    unsigned long long plainLen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plain.data(), &plainLen,
            nullptr,
            ct, ctLen,
            nullptr, 0,
            nonce,
            key.data()) != 0) {
        throw std::runtime_error(
            "Decryption failed (wrong password or tampered data)");
    }
    plain.resize(plainLen);
    return plain;
}

EncryptedTotp EncryptedTotp::load(const std::string& password,
                                  const std::vector<std::uint8_t>& data)
{
    EncryptedTotp tmp(password, otpkit::Config{});
    const auto plain = tmp.decrypt(data);
    const std::string json(plain.begin(), plain.end());
    auto [cfg, secret] = parseJson(json);
    return EncryptedTotp(password, cfg, secret, 0);
}

EncryptedTotp EncryptedTotp::load(const std::string& password,
                                  const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(f)),
         std::istreambuf_iterator<char>());
    return load(password, data);
}

} // namespace otpkit::examples
```

### Usage

```cpp
#include "encrypted_totp.hpp"

int main() {
    const std::string password = "correct horse battery staple";
    otpkit::Config cfg;

    otpkit::examples::EncryptedTotp auth(password, cfg, 20);
    auth.save("myauth.enc");

    auto loaded = otpkit::examples::EncryptedTotp::load(password, "myauth.enc");
    std::cout << "Same code: "
              << (loaded.generateCode() == auth.generateCode() ? "yes" : "no")
              << "\n";
}
```

### CMake

```cmake
find_package(unofficial-sodium CONFIG REQUIRED)

add_executable(my_app main.cpp encrypted_totp.cpp)
target_link_libraries(my_app PRIVATE
    otpkit::core
    unofficial-sodium::sodium
)
```

### Notes

- The file is portable across machines and platforms.
- The user must remember the password. There is no recovery.
- Wrong password → decryption fails with a clear error.
- Argon2id with interactive parameters resists brute-force attacks.
- Salt and nonce are stored with the ciphertext.

---

## Design notes

### Why subclass instead of a `StorageProvider` interface?

A strategy interface (`IStorageProvider`) would allow composition and
dependency injection, but:

- It adds a virtual call on every `save`/`load`.
- It requires lifetime management (`shared_ptr` or references).
- It exposes more surface area in the public API.

For a library that aims to be small, the hook approach is preferable:
two `protected virtual` methods, default identity, override only if
needed.

### Symmetry requirement

If you override `encrypt`, you **must** override `decrypt` symmetrically.
Otherwise, saved files will be unreadable. `otpkit` does not detect
this mismatch.

### Version compatibility

Neither example stores a format version. If you later change the
encryption scheme, old files become unreadable. To migrate safely:

1. Add a version byte at the start of `encrypt` output.
2. On `decrypt`, branch on the version.
3. Keep backward-compatible decrypt for at least one major version.

### Zeroing memory

`otpkit` does not zero out secrets from memory. If you need that:

- Use `sodium_memzero` on your `password_` and any temporary buffers.
- Use `mlock` / `VirtualLock` to prevent swapping.

This is the application's responsibility.
