# Examples have no external dependencies. They only require the core
# library. They are built only when -DOTPKIT_BUILD_EXAMPLES=ON.

add_executable(example_totp example_totp.cpp)
target_link_libraries(example_totp PRIVATE otpkit::core)

add_executable(example_hotp example_hotp.cpp)
target_link_libraries(example_hotp PRIVATE otpkit::core)
