/**
 * @file example_hotp.cpp
 * @brief Minimal HOTP CLI example for otpkit.
 *
 * Demonstrates both stateless and stateful use of HOTP.
 */
#include "otpkit/hotp.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

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

    const std::string path = "myhotp.json";

    std::unique_ptr<Hotp> auth;

    if (std::ifstream(path).good()) {
        auth = std::make_unique<Hotp>(Hotp::load(path));
        std::cout << "Loaded existing HOTP from " << path << "\n\n";
    } else {
        Config cfg;
        auth = std::make_unique<Hotp>(cfg, /*initialCounter=*/0, /*secretBytes=*/20);
        auth->save(path);
        std::cout << "Created new HOTP, saved to " << path << "\n\n";
    }

    std::cout << "Secret (Base32): " << grouped(auth->secretBase32()) << "\n";
    std::cout << "Counter: " << auth->counter() << "\n\n";

    // Stateful mode: show the current code
    std::cout << "Current code: " << auth->generateCode() << "\n";

    std::cout << "Enter the code from your app: ";
    std::string input;
    std::cin >> input;

    if (auth->verify(input, /*window=*/10, /*advanceOnSuccess=*/true)) {
        std::cout << "OK\n";
        std::cout << "New counter: " << auth->counter() << "\n";
        auth->save(path);
        return 0;
    }
    std::cout << "FAIL\n";
    return 1;
}
