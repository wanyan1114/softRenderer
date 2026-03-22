#include "app/Application.h"

#include "app/CameraLayer.h"
#include "app/RenderLayer.h"
#include "app/SceneLayer.h"
#include "render/Camera.h"
#include "render/Framebuffer.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <utility>

namespace sr {
namespace {

constexpr float kFpsReportIntervalSeconds = 0.5f;
const render::Camera kFallbackCamera{};

} // namespace

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)),
      m_Width(width),
      m_Height(height),
      m_Window(m_Title, m_Width, m_Height)
{
    BuildLayers();
}

int Application::Run()
{
    render::Framebuffer framebuffer(m_Width, m_Height);
    StartupState startupState{};
    LayerContext context{};
    context.activeCamera = &kFallbackCamera;
    context.framebuffer = &framebuffer;
    context.input = &m_Window.Input();
    context.startupState = &startupState;
    context.platformName = m_Window.PlatformName();

    m_LayerStack.OnAttach(context);
    if (startupState.shouldExit) {
        if (!startupState.exitMessage.empty()) {
            std::ostream& stream = startupState.exitCode == 0 ? std::cout : std::cerr;
            stream << startupState.exitMessage << '\n';
        }
        m_LayerStack.OnDetach(context);
        return startupState.exitCode;
    }

    if (!CreateWindow()) {
        m_LayerStack.OnDetach(context);
        return 1;
    }

    PrintStartupMessage(startupState);
    m_Window.Show();

    const int exitCode = RunMainLoop(context);
    Shutdown(context);
    return exitCode;
}

void Application::BuildLayers()
{
    m_LayerStack.EmplaceLayer<SceneLayer>();
    m_LayerStack.EmplaceLayer<CameraLayer>(m_Width, m_Height);
    m_LayerStack.EmplaceLayer<RenderLayer>();
}

bool Application::CreateWindow()
{
    if (m_Window.Create()) {
        return true;
    }

    std::cerr << "Failed to create main window on platform: "
              << m_Window.PlatformName() << '\n';
    return false;
}

int Application::RunMainLoop(LayerContext& context)
{
    auto previousFrameTime = std::chrono::steady_clock::now();
    int fpsFrameCount = 0;
    float fpsAccumulatedSeconds = 0.0f;
    bool wroteStatusLine = false;

    while (m_Window.ProcessEvents()) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
        previousFrameTime = currentFrameTime;

        context.activeCamera = &kFallbackCamera;
        context.input = &m_Window.Input();
        m_LayerStack.OnUpdate(deltaTime, context);
        m_LayerStack.OnRender(context);

        if (context.framebuffer == nullptr || !m_Window.Present(*context.framebuffer)) {
            std::cerr << "Failed to present framebuffer to window.\n";
            return 1;
        }

        fpsAccumulatedSeconds += deltaTime;
        ++fpsFrameCount;
        if (fpsAccumulatedSeconds >= kFpsReportIntervalSeconds) {
            const float averageFps = fpsFrameCount > 0 && fpsAccumulatedSeconds > 0.0f
                ? static_cast<float>(fpsFrameCount) / fpsAccumulatedSeconds
                : 0.0f;

            std::cout << "\rFPS: "
                      << std::fixed << std::setprecision(1)
                      << averageFps
                      << std::flush;

            fpsAccumulatedSeconds = 0.0f;
            fpsFrameCount = 0;
            wroteStatusLine = true;
        }
    }

    if (wroteStatusLine) {
        std::cout << '\n';
    }

    return 0;
}

void Application::PrintStartupMessage(const StartupState& startupState) const
{
    std::cout << startupState.startupMessage;
}

void Application::Shutdown(LayerContext& context)
{
    m_LayerStack.OnDetach(context);
}

} // namespace sr

