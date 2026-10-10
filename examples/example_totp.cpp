/**
 * @file example_totp.cpp
 * @brief Minimal TOTP CLI example for otpkit.
 *
 * Creates (or loads) a TOTP authenticator, prints the otpauth URI,
 * then verifies a code entered by the user.
 */
#include "otpkit/totp.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

/// Group a string into blocks of N characters for readability.
std::string grouped(const std::string& s, std::size_t group = 4) {
    std::string out;
    out.reserve(s.size() + s.size() / group);
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i > 0 && i % group == 0) out.push_back(' ');
        out.push_back(s[i]);
    }
    return out;
}

} // namespace

int main() {
    using namespace otpkit;

    const std::string path = "myauth.json";

    std::unique_ptr<Totp> auth;

    if (std::ifstream(path).good()) {
        auth = std::make_unique<Totp>(Totp::load(path));
        std::cout << "Loaded existing authenticator from " << path << "\n\n";
    } else {
        Config cfg;
        auth = std::make_unique<Totp>(cfg, /*secretBytes=*/20);
        auth->save(path);
        std::cout << "Created new authenticator, saved to " << path << "\n\n";
    }

    std::cout << "Secret (Base32): " << grouped(auth->secretBase32()) << "\n";
    std::cout << "URI:\n  "
              << auth->otpauthUri("MyApp", "ali@example.com") << "\n\n";

    std::cout << "Current code: " << auth->generateCode()
              << "  (expires in " << auth->secondsRemaining() << "s)\n";

    std::cout << "Enter the code from your app: ";
    std::string input;
    std::cin >> input;

    if (auth->verify(input)) {
        std::cout << "OK\n";
        return 0;
    }
    std::cout << "FAIL\n";
    return 1;
}
