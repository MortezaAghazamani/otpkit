# otpkit

> Portable C++17 library for **HOTP** and **TOTP** (RFC 4226 / RFC 6238).

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![vcpkg](https://img.shields.io/badge/vcpkg-ready-blue.svg)](https://vcpkg.io)

`otpkit` is a small, dependency-free C++17 library for generating and
verifying one-time passwords compatible with Google Authenticator, Authy,
Microsoft Authenticator, and any RFC-compliant app.

## Features

- **HOTP** (RFC 4226) with both stateful and stateless APIs.
- **TOTP** (RFC 6238) with the standard 30-second period and 6-digit codes.
- **HMAC-SHA1 / SHA256 / SHA512** — implemented internally, no OpenSSL.
- **Base32** encoding/decoding (RFC 4648).
- **`otpauth://` URI** generation for QR scanning or manual entry.
- **Secure random secrets** via `BCryptGenRandom` (Windows) or `/dev/urandom` (POSIX).
- **Pluggable storage**: plain by default; override `encrypt`/`decrypt`
  to add AES, DPAPI, Keychain, or anything else.

## Requirements

- **C++17 compiler**: MSVC 2019+, GCC 9+, or Clang 10+.
- **CMake 3.20+**.
- **vcpkg** for optional dependencies.

## Directory layout

```
otpkit/
├── include/otpkit/     Public API headers
├── src/                Library implementation
├── tests/              Catch2 unit tests
├── examples/           CLI examples
├── docs/               Markdown documentation
└── vcpkg.json          vcpkg manifest
```

## Getting started

### 1. Clone and set up vcpkg

```bash
git clone https://github.com/MortezaAghazamani/otpkit.git
cd otpkit

# If vcpkg is not installed yet:
git clone https://github.com/microsoft/vcpkg.git ../vcpkg
../vcpkg/bootstrap-vcpkg.bat     # Windows
../vcpkg/bootstrap-vcpkg.sh      # Linux
```

### 2. Configure, build, test

**Windows (any Visual Studio version):**

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

cmake -S . -B build `
      -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
      -DVCPKG_MANIFEST_FEATURES="tests" `
      -DOTPKIT_BUILD_TESTS=ON `
      -DOTPKIT_BUILD_EXAMPLES=ON

cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**Linux (any compiler):**

```bash
export VCPKG_ROOT=$HOME/vcpkg

cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_MANIFEST_FEATURES="tests" \
      -DOTPKIT_BUILD_TESTS=ON \
      -DOTPKIT_BUILD_EXAMPLES=ON

cmake --build build -j
ctest --test-dir build --output-on-failure
```

> **Note:** CMake automatically picks the default generator on your
> system. You can force one with `-G "Ninja"`, `-G "Unix Makefiles"`, etc.

### 3. CMake options

| Option                    | Default | Effect |
|---------------------------|---------|--------|
| `OTPKIT_BUILD_TESTS`      | OFF     | Build the unit tests. Requires the vcpkg feature `tests`. |
| `OTPKIT_BUILD_EXAMPLES`   | OFF     | Build the CLI examples. |
| `VCPKG_MANIFEST_FEATURES` | (unset) | vcpkg features to install (`tests`). |

The core library is built by default and has **no external dependencies**.

## Minimal usage

```cpp
#include <otpkit/totp.hpp>
#include <iostream>

int main() {
    otpkit::Config cfg;                        // SHA1, 6 digits, 30 s
    otpkit::Totp   auth(cfg, /*secretBytes=*/20);

    std::cout << "URI:  "
              << auth.otpauthUri("MyApp", "ali@example.com") << "\n";
    std::cout << "Code: " << auth.generateCode() << "\n";

    std::string input;
    std::cin >> input;
    std::cout << (auth.verify(input) ? "OK\n" : "FAIL\n");
}
```

## HOTP usage

```cpp
#include <otpkit/hotp.hpp>

otpkit::Config cfg;
otpkit::Hotp   hotp(cfg, /*initialCounter=*/0, /*secretBytes=*/20);

// Stateless: generate for a specific counter.
std::string code = hotp.generateCode(42);

// Stateful: use and advance the internal counter.
std::string current = hotp.generateCode();
if (hotp.verify(userInput, /*window=*/10, /*advanceOnSuccess=*/true)) {
    // The internal counter is now ahead by one.
}
```

## Persistence

```cpp
// File-based (default: plain JSON).
auth.save("myauth.json");
auto loaded = otpkit::Totp::load("myauth.json");

// In-memory (e.g. database BLOB, registry, network).
std::vector<std::uint8_t> bytes = auth.save();
auto fromBytes = otpkit::Totp::load(bytes);
```

By default, storage is **plain JSON** — readable by anyone with file
access. To protect it, override the `encrypt` / `decrypt` hooks:

```cpp
class MyTotp : public otpkit::Totp {
public:
    MyTotp(const std::string& pwd, otpkit::Config cfg)
        : Totp(cfg, 20), pwd_(pwd) {}

    static MyTotp load(const std::string& pwd, const std::string& path);

protected:
    std::vector<std::uint8_t>
    encrypt(const std::vector<std::uint8_t>& plain) const override {
        return aesGcmEncrypt(pwd_, plain);
    }
    std::vector<std::uint8_t>
    decrypt(const std::vector<std::uint8_t>& cipher) const override {
        return aesGcmDecrypt(pwd_, cipher);
    }

private:
    std::string pwd_;
};
```

See `docs/encryption.md` for complete DPAPI and libsodium examples.

## Documentation

- [Encryption](docs/encryption.md) — DPAPI and libsodium examples for
  protecting secrets at rest.
- [QR codes](docs/qr.md) — Generating QR images from otpauth URIs.
- [ImGui widget](docs/imgui.md) — Building an `ImAuthenticator` widget
  with SDL3 + OpenGL3.

These documents show how to integrate `otpkit` with optional
dependencies. The library itself has no such dependencies.

## Installation

### Using the library in your own project

After building and installing:

```bash
cmake --install build --prefix /path/to/install
```

Use it in another CMake project:

```cmake
find_package(otpkit CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE otpkit::core)
```

### Using vcpkg

`otpkit` will be available as a vcpkg port. Once published:

```json
{
  "dependencies": [ "otpkit" ]
}
```

```cmake
find_package(otpkit CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE otpkit::core)
```

## Security notes

- **Plain storage is not encrypted.** Anyone with file access can read
  the secret. Use `encrypt`/`decrypt` overrides, DPAPI, or libsodium.
- **`otpauthUri()` contains the secret in plaintext.** Do not log or
  transmit it insecurely.
- **Verify uses a symmetric time window** (default ±1 period = ±30 s)
  to tolerate clock skew. Adjust via the `window` parameter.
- **HMAC-SHA1 is the default** for maximum compatibility with consumer
  apps. SHA256/SHA512 are available via `Config::algorithm`.
- **The library does not enforce rate limiting.** Add it in your app.

## Testing

The test suite uses Catch2 and covers:

- SHA-1 / SHA-256 / SHA-512 against NIST vectors.
- HMAC-SHA1 against RFC 2202.
- HMAC-SHA256 against RFC 4231.
- Base32 against RFC 4648.
- HOTP against RFC 4226 test vectors.
- TOTP against RFC 6238 test vectors.

```bash
ctest --test-dir build -C Release --output-on-failure
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## License

MIT — see [LICENSE](LICENSE).

Copyright (c) 2025 Morteza Aghazamani.
