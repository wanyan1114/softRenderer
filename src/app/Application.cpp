#include "app/Application.h"

#include "base/Math.h"
#include "platform/Platform.h"
#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/Renderer.h"
#include "render/Vertex.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace sr {

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)), m_Width(width), m_Height(height)
{
}

int Application::Run()
{
    using namespace std::chrono_literals;

    if (!platform::Platform::Initialize(m_Title, m_Width, m_Height)) {
        std::cerr << "Failed to create main window on platform: "
                  << platform::Platform::Name() << '\n';
        platform::Platform::Close();
        return 1;
    }

    render::Framebuffer framebuffer(m_Width, m_Height);
    render::Renderer renderer(framebuffer);

    const render::Mesh mesh{
        render::Triangle{
            { math::Vec2{ -0.65f, -0.55f }, render::Color{ 255, 119, 71 } },
            { math::Vec2{  0.65f, -0.55f }, render::Color{ 255, 119, 71 } },
            { math::Vec2{  0.0f,   0.70f }, render::Color{ 255, 119, 71 } },
        },
    };

    std::cout << "SoftRenderer skeleton started on " << platform::Platform::Name()
              << " with window \"" << m_Title << "\" ("
              << m_Width << "x" << m_Height << ")\n";

    platform::Platform::Show();

    while (platform::Platform::ProcessEvents()) {
        renderer.Clear(render::Color{ 18, 24, 38 });
        renderer.Draw(mesh);

        if (!platform::Platform::Present(framebuffer)) {
            std::cerr << "Failed to present framebuffer to window.\n";
            platform::Platform::Close();
            return 1;
        }

        std::this_thread::sleep_for(16ms);
    }

    platform::Platform::Close();
    return 0;
}

} // namespace sr
