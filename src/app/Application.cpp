#include "app/Application.h"

#include "platform/Platform.h"
#include "platform/Window.h"

#include <iostream>

namespace sr {

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)), m_Width(width), m_Height(height)
{
}

int Application::Run()
{
    platform::Initialize();

    platform::Window window(m_Title, m_Width, m_Height);
    if (!window.Create()) {
        std::cerr << "Failed to create main window on platform: "
                  << platform::Name() << '\n';
        platform::Shutdown();
        return 1;
    }

    std::cout << "SoftRenderer skeleton started on " << platform::Name()
              << " with window \"" << window.Title() << "\" ("
              << window.Width() << "x" << window.Height() << ")\n";

    platform::Shutdown();
    return 0;
}

} // namespace sr
