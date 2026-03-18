#pragma once

#include <string>

namespace sr::render {
class Framebuffer;
}

namespace sr::platform {

class Window {
public:
    Window(std::string title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create();
    void Show();
    bool ProcessEvents();
    bool Present(const render::Framebuffer& framebuffer) const;

    bool IsClosed() const { return m_Closed; }

    const std::string& Title() const { return m_Title; }
    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

private:
    std::string m_Title;
    int m_Width;
    int m_Height;
    void* m_Handle;
    bool m_Closed;
};

} // namespace sr::platform
