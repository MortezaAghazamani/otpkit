/**
 * @file random.hpp
 * @brief Platform-independent secure random source (internal header).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace otpkit::platform {

/**
 * @brief Return @p bytes cryptographically secure random bytes.
 *
 * Implemented per-platform:
 *  - Windows: BCryptGenRandom
 *  - POSIX:   /dev/urandom
 *
 * @throws std::runtime_error if the underlying source fails.
 */
std::vector<std::uint8_t> secureRandom(std::size_t bytes);

} // namespace otpkit::platform
