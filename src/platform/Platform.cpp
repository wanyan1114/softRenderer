#include "platform/Platform.h"

#include "platform/Window.h"

#include <windows.h>

#include <memory>
#include <utility>

namespace sr::platform {

namespace {

std::unique_ptr<Window> g_MainWindow;

} // namespace

bool Platform::Initialize(const std::string& title, int width, int height)
{
    SetProcessDPIAware();

    g_MainWindow = std::make_unique<Window>(title, width, height);
    if (!g_MainWindow->Create()) {
        g_MainWindow.reset();
        return false;
    }

    return true;
}

void Platform::Show()
{
    if (g_MainWindow) {
        g_MainWindow->Show();
    }
}

bool Platform::ProcessEvents()
{
    if (!g_MainWindow) {
        return false;
    }

    return g_MainWindow->ProcessEvents();
}

bool Platform::Present(const render::Framebuffer& framebuffer)
{
    if (!g_MainWindow) {
        return false;
    }

    return g_MainWindow->Present(framebuffer);
}

void Platform::Close()
{
    g_MainWindow.reset();
}

const char* Platform::Name()
{
    return "Windows";
}

} // namespace sr::platform
