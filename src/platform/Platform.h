#pragma once

#include <string>

namespace sr::render {
class Framebuffer;
}

namespace sr::platform {

class Platform {
public:
    static bool Initialize(const std::string& title, int width, int height);
    static void Show();
    static bool ProcessEvents();
    static bool Present(const render::Framebuffer& framebuffer);
    static void Close();
    static const char* Name();

private:
    Platform() = delete;
};

} // namespace sr::platform
