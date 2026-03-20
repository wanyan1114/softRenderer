#pragma once

#include "platform/Input.h"

#include <array>
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
    bool IsKeyDown(Key key) const;
    bool OnMessage(unsigned int message, unsigned long long wParam, long long lParam, long long& result);

    bool IsClosed() const { return m_Closed; }

    const std::string& Title() const { return m_Title; }
    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

private:
    void SetKeyState(Key key, bool isDown);
    void ResetInput();

    std::string m_Title;
    int m_Width;
    int m_Height;
    void* m_Handle;
    bool m_Closed;
    std::array<bool, static_cast<std::size_t>(Key::Count)> m_KeyStates;
};

} // namespace sr::platform
