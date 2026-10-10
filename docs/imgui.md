# Building an ImGui widget for otpkit

`otpkit` provides the core TOTP logic but no UI. This document shows how
to build an `ImAuthenticator` widget using [Dear ImGui](https://github.com/ocornut/imgui)
with the **SDL3** and **OpenGL3** backends.

The widget renders:

- The issuer and account label.
- The current 6-digit code.
- A live progress bar showing time until expiry.
- An input field for the user-entered code.
- A status line ("OK" / "Wrong or expired code").

---

## 1. Dependencies

With vcpkg, add to your `vcpkg.json`:

```json
{
  "dependencies": [
    {
      "name": "imgui",
      "default-features": false,
      "features": [ "sdl3-binding", "opengl3-binding" ]
    },
    "sdl3"
  ]
}
```

vcpkg installs ImGui with the SDL3 and OpenGL3 backends already compiled.

---

## 2. Header

### `im_authenticator.hpp`

```cpp
#pragma once

#include "otpkit/totp.hpp"

#include <string>

namespace myapp::imgui {

enum class AuthState {
    Idle,
    Accepted,
    Rejected
};

struct AuthStateStore {
    char      buffer[16] = {};
    AuthState state      = AuthState::Idle;
};

struct Options {
    bool autoSubmitOnEnter = true;
};

/// Render the authenticator widget.
/// Returns true if the user just submitted a valid code.
bool ImAuthenticator(otpkit::Totp& totp,
                     const std::string& issuer,
                     const std::string& account,
                     AuthStateStore& store,
                     const Options& options = {});

} // namespace myapp::imgui
```

---

## 3. Implementation

### `im_authenticator.cpp`

```cpp
#include "im_authenticator.hpp"

#include <imgui.h>

#include <cstring>

namespace myapp::imgui {

bool ImAuthenticator(otpkit::Totp& totp,
                     const std::string& issuer,
                     const std::string& account,
                     AuthStateStore& store,
                     const Options& options)
{
    bool accepted = false;

    ImGui::PushID(&totp);

    // Header
    ImGui::TextUnformatted(issuer.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", account.c_str());
    ImGui::Separator();

    // Current code
    const std::string code = totp.generateCode();
    ImGui::TextUnformatted("Code:");
    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextUnformatted(code.c_str());
    ImGui::SetWindowFontScale(1.0f);

    // Progress bar
    const int remaining = totp.secondsRemaining();
    const int period    = totp.config().period;
    const float frac    = static_cast<float>(remaining) /
                          static_cast<float>(period);

    ImVec4 color(0.2f, 0.8f, 0.2f, 1.0f);
    if (remaining <= 5)       color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
    else if (remaining <= 10) color = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f));
    ImGui::PopStyleColor();
    ImGui::Text("%d second(s) remaining", remaining);

    // Input
    ImGui::Separator();
    ImGui::TextUnformatted("Enter the code from your app:");

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsDecimal;
    if (options.autoSubmitOnEnter) {
        flags |= ImGuiInputTextFlags_EnterReturnsTrue;
    }

    const bool pressedEnter = ImGui::InputText(
        "##code", store.buffer, sizeof(store.buffer), flags);

    ImGui::SameLine();
    const bool pressedVerify = ImGui::Button("Verify");

    if (pressedEnter || pressedVerify) {
        if (totp.verify(store.buffer)) {
            store.state = AuthState::Accepted;
            accepted = true;
        } else {
            store.state = AuthState::Rejected;
        }
        std::memset(store.buffer, 0, sizeof(store.buffer));
    }

    // Status
    switch (store.state) {
        case AuthState::Idle:
            ImGui::TextDisabled("Waiting for input.");
            break;
        case AuthState::Accepted:
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "OK");
            break;
        case AuthState::Rejected:
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                               "Wrong or expired code.");
            break;
    }

    ImGui::PopID();
    return accepted;
}

} // namespace myapp::imgui
```

---

## 4. Hosting the widget: SDL3 + OpenGL3

```cpp
#include "im_authenticator.hpp"
#include "otpkit/totp.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

int main(int, char**) {
    // ---- SDL3 init ----
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;

    const char* glslVersion = "#version 130";

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "otpkit demo", 800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    // ---- ImGui init ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    // ---- otpkit ----
    otpkit::Config cfg;
    otpkit::Totp auth(cfg, 20);
    myapp::imgui::AuthStateStore store;

    // ---- Main loop ----
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Authenticator");
        const bool ok = myapp::imgui::ImAuthenticator(
            auth, "MyApp", "ali@example.com", store);
        if (ok) ImGui::OpenPopup("Welcome");

        if (ImGui::BeginPopupModal("Welcome", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Authentication succeeded.");
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::End();

        ImGui::Render();
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // ---- Cleanup ----
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

### CMake

```cmake
find_package(imgui CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(my_app
    main.cpp
    im_authenticator.cpp
)

target_link_libraries(my_app PRIVATE
    otpkit::core
    imgui::imgui
    SDL3::SDL3
    OpenGL::GL
)
```

---

## 5. Notes

### No QR by default

The widget intentionally does not render a QR code. If you want one,
combine this widget with a QR library (see `docs/qr.md`) and display the
generated bitmap via `ImGui::Image`.

### Font scaling

`ImGui::SetWindowFontScale` is applied to the code to make it more
readable. You can adjust or remove it based on your style.

### Auto-submit

`autoSubmitOnEnter` is enabled by default: pressing Enter in the input
field submits the code. Disable it if you want an explicit button press.

### Thread safety

The widget calls `totp.generateCode()` and `totp.verify()` from the UI
thread. Both are safe as long as no other thread mutates the same `Totp`
instance. If you persist the code in another thread, serialize access
externally.
