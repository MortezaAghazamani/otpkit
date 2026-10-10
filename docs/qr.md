# Generating QR codes for otpauth URIs

`otpkit` produces `otpauth://` URIs that can be scanned by any
TOTP-compatible app (Google Authenticator, Authy, Microsoft
Authenticator, etc.). The library itself does **not** generate QR
images — that is a separate concern with its own dependencies.

This document shows how to render a QR code from an otpauth URI using
[nayuki-qr-code-generator](https://github.com/nayuki/QR-Code-generator),
a small, dependency-free C++ library.

---

## 1. Install qrcodegen

### vcpkg

```json
"dependencies": [ "nayuki-qr-code-generator" ]
```

### Manual

Clone the repository and copy these two files into your project:

- `cpp/qrcodegen.hpp`
- `cpp/qrcodegen.cpp`

---

## 2. Generate a QR matrix

```cpp
#include "otpkit/totp.hpp"
#include "qrcodegen.hpp"

#include <string>
#include <vector>

struct QrMatrix {
    int size = 0;
    std::vector<std::uint8_t> modules;   // size * size, 0 or 1

    bool at(int x, int y) const {
        return modules[y * size + x] != 0;
    }
};

QrMatrix makeQr(const std::string& text) {
    using qrcodegen::QrCode;

    const QrCode qr = QrCode::encodeText(text.c_str(),
                                         QrCode::Ecc::MEDIUM);

    QrMatrix out;
    out.size = qr.getSize();
    out.modules.resize(out.size * out.size);

    for (int y = 0; y < out.size; ++y) {
        for (int x = 0; x < out.size; ++x) {
            out.modules[y * out.size + x] = qr.getModule(x, y) ? 1 : 0;
        }
    }
    return out;
}
```

---

## 3. Render as ASCII (for CLI / debugging)

```cpp
#include <iostream>

void printAscii(const QrMatrix& m, int border = 2) {
    const int total = m.size + border * 2;
    for (int y = 0; y < total; ++y) {
        for (int x = 0; x < total; ++x) {
            const int mx = x - border;
            const int my = y - border;
            const bool dark =
                (mx >= 0 && my >= 0 && mx < m.size && my < m.size) &&
                m.at(mx, my);
            std::cout << (dark ? "\xE2\x96\x88\xE2\x96\x88" : "  ");
        }
        std::cout << "\n";
    }
}
```

**Note:** ASCII rendering depends on the terminal font. Many fonts
render block characters imperfectly. For reliable scanning, use a real
bitmap and display it in a GUI.

---

## 4. Render as a grayscale bitmap

```cpp
#include <vector>

std::vector<std::uint8_t>
toGrayscale(const QrMatrix& m, int scale, int border,
            int& outWidth, int& outHeight)
{
    outWidth  = (m.size + border * 2) * scale;
    outHeight = outWidth;

    std::vector<std::uint8_t> img(outWidth * outHeight, 255);

    for (int y = 0; y < m.size; ++y) {
        for (int x = 0; x < m.size; ++x) {
            if (!m.at(x, y)) continue;
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    const int px = (x + border) * scale + dx;
                    const int py = (y + border) * scale + dy;
                    img[py * outWidth + px] = 0;
                }
            }
        }
    }
    return img;
}
```

The output can be uploaded to a GPU as a texture, written to a PNG file
(via libpng or stb_image_write), or displayed anywhere.

---

## 5. Complete example

```cpp
#include "otpkit/totp.hpp"
#include "qrcodegen.hpp"

#include <iostream>

int main() {
    otpkit::Config cfg;
    otpkit::Totp   auth(cfg, 20);

    const std::string uri = auth.otpauthUri("MyApp", "ali@example.com");
    std::cout << "URI: " << uri << "\n\n";

    const auto matrix = makeQr(uri);
    printAscii(matrix);
}
```

### CMake

```cmake
find_package(unofficial-nayuki-qr-code-generator CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    otpkit::core
    unofficial::nayuki-qr-code-generator::nayuki-qr-code-generator
)
```

> **Note:** vcpkg names its CMake targets with a double-colon
> namespace: `unofficial::<port>::<port>`. This differs from the
> `find_package` name which uses hyphens.

---

## 6. Choosing an error correction level

qrcodegen supports four levels:

| Level    | Recovery | Notes |
|----------|----------|-------|
| `LOW`    | ~7%      | Smallest matrix |
| `MEDIUM` | ~15%     | Recommended default |
| `QUARTILE` | ~25%   | Larger, more robust |
| `HIGH`   | ~30%     | Most robust, largest |

For otpauth URIs, `MEDIUM` is a good default. Use `HIGH` if the QR will
be printed on small surfaces or displayed on a screen with glare.
