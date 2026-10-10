# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial implementation of HOTP (RFC 4226) and TOTP (RFC 6238).
- Self-contained SHA-1, SHA-256, SHA-512 implementations.
- Generic HMAC over SHA-1 / SHA-256 / SHA-512.
- Base32 encode/decode (RFC 4648, unpadded output).
- Secure random secrets via `BCryptGenRandom` (Windows) and
  `/dev/urandom` (POSIX).
- `otpauth://` URI generation compatible with Google Authenticator,
  Authy, and Microsoft Authenticator.
- Persistence hooks (`encrypt` / `decrypt`) for pluggable storage.
- File-based and in-memory persistence (`save` / `load` overloads).
- Two CLI examples: `example_totp` and `example_hotp`.
- Unit tests covering RFC 4226, RFC 6238, RFC 4648, RFC 2202, RFC 4231,
  and NIST SHA vectors.
- CMake package export for `find_package(otpkit)`.

### Documentation
- `docs/encryption.md`: DPAPI and libsodium examples.
- `docs/qr.md`: QR generation with nayuki-qr-code-generator.
- `docs/imgui.md`: ImGui widget with SDL3 + OpenGL3.

[Unreleased]: https://github.com/MortezaAghazamani/otpkit/compare/HEAD
