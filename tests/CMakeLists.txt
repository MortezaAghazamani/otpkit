find_package(Catch2 3 REQUIRED CONFIG)

add_executable(otpkit_tests
    test_sha.cpp
    test_hmac.cpp
    test_base32.cpp
    test_rfc4226_hotp.cpp
    test_rfc6238_totp.cpp
)

target_link_libraries(otpkit_tests PRIVATE
    otpkit::core
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(otpkit_tests)
