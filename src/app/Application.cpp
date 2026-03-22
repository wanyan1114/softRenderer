#include "app/Application.h"

#include "app/layer/CameraLayer.h"
#include "app/layer/RenderLayer.h"
#include "app/layer/SceneLayer.h"
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
    using clock = std::chrono::steady_clock;
    constexpr float kMillisecondsPerSecond = 1000.0f;

    auto previousFrameTime = clock::now();
    int fpsFrameCount = 0;
    float fpsAccumulatedSeconds = 0.0f;
    float frameAccumulatedSeconds = 0.0f;
    float renderAccumulatedSeconds = 0.0f;
    float presentAccumulatedSeconds = 0.0f;
    bool wroteStatusLine = false;

    while (m_Window.ProcessEvents()) {
        const auto frameStartTime = clock::now();
        const float deltaTime = std::chrono::duration<float>(frameStartTime - previousFrameTime).count();
        previousFrameTime = frameStartTime;

        const auto renderStartTime = clock::now();
        context.activeCamera = &kFallbackCamera;
        context.input = &m_Window.Input();
        m_LayerStack.OnUpdate(deltaTime, context);
        m_LayerStack.OnRender(context);
        const auto renderEndTime = clock::now();

        const auto presentStartTime = renderEndTime;
        if (context.framebuffer == nullptr || !m_Window.Present(*context.framebuffer)) {
            std::cerr << "Failed to present framebuffer to window.\n";
            return 1;
        }
        const auto presentEndTime = clock::now();

        const float frameSeconds = std::chrono::duration<float>(presentEndTime - frameStartTime).count();
        const float renderSeconds = std::chrono::duration<float>(renderEndTime - renderStartTime).count();
        const float presentSeconds = std::chrono::duration<float>(presentEndTime - presentStartTime).count();

        fpsAccumulatedSeconds += frameSeconds;
        frameAccumulatedSeconds += frameSeconds;
        renderAccumulatedSeconds += renderSeconds;
        presentAccumulatedSeconds += presentSeconds;
        ++fpsFrameCount;
        if (fpsAccumulatedSeconds >= kFpsReportIntervalSeconds) {
            const float averageFps = fpsFrameCount > 0 && fpsAccumulatedSeconds > 0.0f
                ? static_cast<float>(fpsFrameCount) / fpsAccumulatedSeconds
                : 0.0f;
            const float averageFrameMilliseconds = fpsFrameCount > 0
                ? frameAccumulatedSeconds * kMillisecondsPerSecond / static_cast<float>(fpsFrameCount)
                : 0.0f;
            const float averageRenderMilliseconds = fpsFrameCount > 0
                ? renderAccumulatedSeconds * kMillisecondsPerSecond / static_cast<float>(fpsFrameCount)
                : 0.0f;
            const float averagePresentMilliseconds = fpsFrameCount > 0
                ? presentAccumulatedSeconds * kMillisecondsPerSecond / static_cast<float>(fpsFrameCount)
                : 0.0f;

            std::cout << "\rFPS: "
                      << std::fixed << std::setprecision(1)
                      << averageFps
                      << " | Frame: " << averageFrameMilliseconds << " ms"
                      << " | Render: " << averageRenderMilliseconds << " ms"
                      << " | Present: " << averagePresentMilliseconds << " ms"
                      << std::flush;

            fpsAccumulatedSeconds = 0.0f;
            frameAccumulatedSeconds = 0.0f;
            renderAccumulatedSeconds = 0.0f;
            presentAccumulatedSeconds = 0.0f;
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



