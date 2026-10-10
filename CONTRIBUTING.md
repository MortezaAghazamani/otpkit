# Contributing to otpkit

First off, thank you for considering a contribution to `otpkit`. This
document explains the development workflow, coding standards, and
submission process so that your work can be reviewed and merged quickly.

If anything here is unclear, open an issue with the `question` label and
we will improve this document.

---

## Table of contents

- [Code of conduct](#code-of-conduct)
- [Prerequisites](#prerequisites)
- [Getting the source](#getting-the-source)
- [Setting up vcpkg](#setting-up-vcpkg)
- [Configuring the build](#configuring-the-build)
- [Building](#building)
- [Running the tests](#running-the-tests)
- [Project layout](#project-layout)
- [Coding style](#coding-style)
- [Adding a new feature](#adding-a-new-feature)
- [Adding a new test](#adding-a-new-test)
- [Commit messages](#commit-messages)
- [Pull request process](#pull-request-process)
- [Documentation](#documentation)
- [Reporting bugs](#reporting-bugs)
- [Security issues](#security-issues)
- [License](#license)

---

## Code of conduct

Be respectful. Assume good faith. Disagree about technical decisions,
not about people. Harassment, insults, or discriminatory language will
not be tolerated under any circumstances.

---

## Prerequisites

You need the following tools installed before you can build `otpkit`:

- **CMake 3.20** or newer.
- **A C++17 compiler**:
  - Windows: MSVC 2019 (v16.8) or newer.
  - Linux: GCC 9 or newer, or Clang 10 or newer.
- **vcpkg** for the optional `catch2` dependency (needed for tests).
- **Git** with a configured user name and email.

Optional, but recommended:

- **clang-format 14 or newer** for formatting.
- **Ninja** for faster builds on Linux.

Verify your toolchain:

```bash
cmake --version        # must be >= 3.20
clang-format --version # optional
```

---

## Getting the source

```bash
git clone https://github.com/MortezaAghazamani/otpkit.git
cd otpkit
```

If you are contributing a fix, work on a feature branch:

```bash
git checkout -b feature/short-description
# or
git checkout -b fix/short-description
```

---

## Setting up vcpkg

`otpkit` uses **vcpkg manifest mode**. The manifest file `vcpkg.json`
declares optional dependencies under named features. Tests require the
`tests` feature, which brings in `catch2`.

### Windows

```powershell
# Clone vcpkg once, anywhere on disk.
git clone https://github.com/microsoft/vcpkg.git D:\vcpkg
cd D:\vcpkg
.\bootstrap-vcpkg.bat

# Remember the location for every new shell session.
$env:VCPKG_ROOT = "D:\vcpkg"
```

### Linux

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

export VCPKG_ROOT=$HOME/vcpkg
```

If you want `VCPKG_ROOT` to persist across shell sessions, add the
`export` line to your shell profile (`~/.bashrc`, `~/.zshrc`, or the
PowerShell profile).

---

## Configuring the build

`otpkit` builds only the core library by default. Tests and examples
are opt-in via command-line flags and vcpkg features.

### Common configurations

**Core library only (default):**

```bash
cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

**Core + tests:**

```bash
cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_MANIFEST_FEATURES="tests" \
      -DOTPKIT_BUILD_TESTS=ON
```

**Core + tests + examples:**

```bash
cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_MANIFEST_FEATURES="tests" \
      -DOTPKIT_BUILD_TESTS=ON \
      -DOTPKIT_BUILD_EXAMPLES=ON
```

On Windows (PowerShell), the same command uses backticks for line
continuation:

```powershell
cmake -S . -B build `
      -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
      -DVCPKG_MANIFEST_FEATURES="tests" `
      -DOTPKIT_BUILD_TESTS=ON `
      -DOTPKIT_BUILD_EXAMPLES=ON
```

### Available options

| Option                    | Default | Effect |
|---------------------------|---------|--------|
| `OTPKIT_BUILD_TESTS`      | OFF     | Build the unit test target. Requires the `tests` vcpkg feature. |
| `OTPKIT_BUILD_EXAMPLES`   | OFF     | Build the CLI examples in `examples/`. |

### Available vcpkg features

| Feature  | Effect |
|----------|--------|
| `tests`  | Pulls in `catch2`. Automatically enables `OTPKIT_BUILD_TESTS`. |

> **Note:** `VCPKG_MANIFEST_FEATURES` controls what vcpkg **installs**,
> while `OTPKIT_BUILD_*` controls what CMake **builds**. Both must be set
> for tests to be available.

---

## Building

```bash
cmake --build build --config Release --parallel
```

On Linux with Ninja, `--config` is not needed:

```bash
cmake --build build --parallel
```

Build products appear in:

- Windows: `build/Release/` and `build/tests/Release/`.
- Linux: `build/` and `build/tests/`.

---

## Running the tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

On Linux, `-C Release` can be omitted. To see the list of tests:

```bash
ctest --test-dir build -N
```

To run a specific test by name:

```bash
ctest --test-dir build -R "RFC 4226" --output-on-failure
```

You can also run the test binary directly (with full Catch2 options):

```bash
./build/tests/otpkit_tests "RFC 6238*" -s
```

> **Reminder:** If tests are not built, `ctest` will report no tests
> found. Reconfigure with `-DVCPKG_MANIFEST_FEATURES="tests"` and
> `-DOTPKIT_BUILD_TESTS=ON`.

---

## Project layout

```
otpkit/
├── CMakeLists.txt          Top-level CMake
├── vcpkg.json              vcpkg manifest
├── include/otpkit/         Public API headers
├── src/
│   ├── core/               SHA, HMAC, Base32
│   ├── hotp.cpp            HOTP implementation
│   ├── totp.cpp            TOTP implementation
│   └── platform/           Platform-specific random
├── tests/                  Catch2 unit tests
├── examples/               CLI examples
├── docs/                   Markdown documentation
└── cmake/                  CMake install helpers
```

### What belongs where

- **`include/otpkit/`** — only public headers. Users include these.
  Every symbol here is part of the stable API.
- **`src/core/`** — pure algorithm code with no platform dependency.
- **`src/platform/`** — the only place where `#ifdef _WIN32` (or
  equivalent) is allowed.
- **`tests/`** — one file per area (`test_sha.cpp`, `test_hmac.cpp`, ...).
- **`examples/`** — small, self-contained programs that demonstrate a
  single concept. No heavy dependencies.
- **`docs/`** — markdown explanations, code snippets, tutorials.

---

## Coding style

`otpkit` follows the rules below. `.clang-format` is provided and
enforced in CI.

### Formatting

- **Indentation:** 4 spaces, no tabs.
- **Line length:** 90 columns (soft limit).
- **Braces:** attach to the preceding line (`if (...) {`).
- **Pointers:** `int* p` (pointer binds to type).

Run the formatter before committing:

```bash
clang-format -i $(find include src tests examples -name '*.cpp' -o -name '*.hpp')
```

### Naming

- **Types:** `PascalCase` (`Totp`, `Config`, `QrMatrix`).
- **Functions and methods:** `camelCase` (`generateCode`, `otpauthUri`).
- **Variables:** `snake_case_with_trailing_underscore_` for members,
  `camelCase` for locals.
- **Constants:** `PascalCase` for `constexpr` (`DIGEST_SIZE`).
- **Namespaces:** lowercase (`otpkit`, `otpkit::platform`).

### Types

- Use fixed-width integer types from `<cstdint>`:
  `std::uint8_t`, `std::uint32_t`, `std::uint64_t`.
- Prefer `std::array` and `std::vector` over C arrays.
- Never use raw `new` or `delete`; use smart pointers or values.
- Use `nullptr`, never `NULL` or `0`.

### Headers

- Every public header uses `#pragma once`.
- Public symbols are documented with Doxygen:
  - `@brief`, `@param`, `@return`, `@throws`.
- Private implementations never leak into public headers.
- Include order:
  1. Corresponding header for a `.cpp` file.
  2. Project headers (`"otpkit/..."`).
  3. Third-party headers (`<catch2/...>`).
  4. Standard library headers.
  5. Platform headers (`<windows.h>`).

### Modern C++

- Use `auto` when the type is obvious from the right side.
- Prefer range-based `for` when iterating containers.
- Use `constexpr` where possible.
- Use `[[nodiscard]]` on functions whose return value must not be
  ignored.
- Avoid `std::endl`; use `'\n'` instead (unless you need a flush).
- Avoid `using namespace` in headers. In `.cpp` files it is acceptable
  inside a function, not at file scope.

---

## Adding a new feature

1. **Open an issue first.** Describe the use case, proposed API, and
   alternatives. This avoids wasted work if the feature is out of scope.
2. **Update the public header.** Add Doxygen comments for every new
   symbol. Keep the API minimal.
3. **Implement in `src/`.** Never put implementation in a public header
   unless it is `inline` and trivial.
4. **Add tests.** See the next section.
5. **Update `CHANGELOG.md`.** Add an entry under `[Unreleased]`.
6. **Update documentation** if the feature is user-visible. Add or
   modify a file under `docs/` if needed.
7. **Open a pull request.**

### Platform-specific features

If your feature is platform-specific (e.g., DPAPI on Windows):

- Put the implementation in `src/platform/` with a `.cpp` per platform.
- Add the source file to `CMakeLists.txt` under the appropriate
  `if(WIN32)` / `elseif(UNIX)` block.
- Keep the public API platform-independent.

---

## Adding a new test

Tests use [Catch2 v3](https://github.com/catchorg/Catch2). Follow these
rules:

1. Create a new file `tests/test_<area>.cpp` or add to an existing one.
2. Register the file in `tests/CMakeLists.txt` under `add_executable`.
3. Use `TEST_CASE` with a short, descriptive name and a tag:

```cpp
#include <catch2/catch_test_macros.hpp>

#include "otpkit/base32.hpp"

TEST_CASE("Base32 encodes RFC 4648 vectors", "[base32]") {
    // ...
}
```

4. Prefer **RFC test vectors** over hand-crafted values:
   - SHA: NIST FIPS 180-4.
   - HMAC: RFC 2202 and RFC 4231.
   - Base32: RFC 4648.
   - HOTP: RFC 4226.
   - TOTP: RFC 6238.

5. Use `REQUIRE` for assertions that must hold. Use `CHECK` for
   multiple assertions where you want all to run.

6. If a test needs a helper, put it in an anonymous namespace at the
   top of the file:

```cpp
namespace {
std::vector<std::uint8_t> bytes(const std::string& s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
}
}
```

7. Keep tests deterministic. Never depend on the system clock or random
   data unless you explicitly seed them.

---

## Commit messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/)
style loosely. Keep the subject line under 72 characters and use the
imperative mood.

### Format

```
<type>: <short summary>

<optional body>

<optional footer>
```

### Types

- `feat` — new feature.
- `fix` — bug fix.
- `docs` — documentation only.
- `test` — tests added or updated.
- `refactor` — code change that does not affect behavior.
- `perf` — performance improvement.
- `build` — build system or dependency changes.
- `ci` — CI configuration changes.
- `chore` — other maintenance.

### Examples

```
feat: add SHA-512 support to HMAC
fix: correct big-endian encoding for HOTP counters
docs: add QR module example
test: add RFC 6238 SHA512 vectors
build: link bcrypt on Windows
ci: cache vcpkg binary artifacts
```

### What to avoid

- `Fixed bug` (no context)
- `Update` (what was updated?)
- `WIP` (do not push WIP commits; squash them first)

### Sign-off

Not required, but you may add a `Signed-off-by:` line if your
organization requires it.

---

## Pull request process

1. **Sync with main** before opening a PR:

   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **Push your branch** and open the PR against `main`.

3. **Fill the PR template.** Describe what, why, and how.

4. **Ensure CI passes.** Six checks must be green:
   - Core / Windows MSVC
   - Core / Ubuntu GCC
   - Full / Windows MSVC
   - Full / Ubuntu GCC
   - Full / Ubuntu Clang
   - Format check

5. **Address review comments.** Push additional commits; do not force
   push after a review has started unless asked.

6. **Squash merge** is the default. Keep the final commit message
   clean and descriptive.

### What reviewers look for

- Correctness (tests pass, edge cases handled).
- API design (does the new symbol belong in the public header?).
- Documentation (Doxygen on public symbols).
- Consistency with existing code.
- Minimal diff (no unrelated changes).

---

## Documentation

- **Public headers** must have Doxygen comments.
- **`docs/`** contains longer explanations with complete examples.
- **`README.md`** is the entry point; keep it concise.
- **`CHANGELOG.md`** must be updated for user-visible changes.

### Doxygen style

```cpp
/**
 * @brief Generate the current TOTP code.
 *
 * Uses the system clock to compute the time step.
 *
 * @return A zero-padded numeric string of length `Config::digits`.
 */
std::string generateCode() const;
```

- One `@brief` per symbol.
- Document every parameter.
- Document exceptions with `@throws`.
- Use present tense ("Returns the...", not "Return the...").

---

## Reporting bugs

Open an issue using the **bug report** template. Include:

- A **minimal reproduction** — the smallest code that shows the bug.
- The exact behavior you expected and observed.
- Full error messages and, if possible, a stack trace.
- Your environment:
  - OS and version.
  - Compiler and version.
  - otpkit version or commit hash.
  - CMake version.
  - vcpkg commit.

Bugs without a reproduction are hard to fix. If you cannot provide one,
describe the scenario in as much detail as possible.

---

## Security issues

Do **not** open a public issue for security vulnerabilities. This
includes (but is not limited to):

- Weak randomness in secret generation.
- Side-channel leakage in `verify`.
- Timing attacks in string comparison.
- Memory disclosure via serialization.
- Any issue that could allow an attacker to forge codes.

Instead, report privately:

- Use [GitHub Security Advisories](https://docs.github.com/en/code-security/security-advisories).
- Or email the maintainer directly.

You will receive an acknowledgment within 72 hours. We aim to release a
fix within 30 days of confirmation.

---

## License

By contributing to `otpkit`, you agree that your contributions will be
licensed under the **MIT License**, the same as the project. See
[`LICENSE`](../LICENSE) for details.

You do not need to sign a Contributor License Agreement (CLA). Just
submit your pull request.

---

Thank you for taking the time to contribute. Every improvement —
even a typo fix — makes `otpkit` better for everyone.
