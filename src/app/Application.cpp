#include "app/Application.h"

#include "app/CameraLayer.h"
#include "app/RenderLayer.h"
#include "platform/Platform.h"
#include "render/Camera.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#ifndef SR_PROJECT_ROOT
#define SR_PROJECT_ROOT "."
#endif

namespace sr {
namespace {

constexpr float kFpsReportIntervalSeconds = 0.5f;
const render::Camera kFallbackCamera{};

std::filesystem::path GetDefaultModelPath()
{
    const char* overridePath = std::getenv("SR_MODEL_PATH");
    if (overridePath != nullptr && overridePath[0] != '\0') {
        return std::filesystem::path{ overridePath };
    }

    return std::filesystem::path{ SR_PROJECT_ROOT } / "assets/models/cube.obj";
}

bool IsObjPath(const std::filesystem::path& path)
{
    return path.extension() == ".obj" || path.extension() == ".OBJ";
}

} // namespace

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)),
      m_Width(width),
      m_Height(height),
      m_Framebuffer(width, height)
{
    BuildLayers();
}

int Application::Run()
{
    PrepareScene();
    if (m_ShouldExitOnStartup) {
        if (!m_StartupExitMessage.empty()) {
            std::ostream& stream = m_StartupExitCode == 0 ? std::cout : std::cerr;
            stream << m_StartupExitMessage << '\n';
        }
        return m_StartupExitCode;
    }

    m_LayerContext.activeCamera = &kFallbackCamera;
    m_LayerContext.sceneView = &m_SceneView;
    m_LayerContext.framebuffer = &m_Framebuffer;
    m_LayerStack.OnAttach(m_LayerContext);

    if (!InitializePlatform()) {
        m_LayerStack.OnDetach(m_LayerContext);
        return 1;
    }

    PrintStartupMessage();
    platform::Platform::Show();

    const int exitCode = RunMainLoop();
    Shutdown();
    return exitCode;
}

void Application::BuildLayers()
{
    m_LayerStack.EmplaceLayer<CameraLayer>(m_Width, m_Height);
    m_LayerStack.EmplaceLayer<RenderLayer>();
}

void Application::PrepareScene()
{
    m_ModelPath = GetDefaultModelPath();
    if (!IsObjPath(m_ModelPath)) {
        RequestStartupExit(1, "Only OBJ rendering is supported in the current version: " + m_ModelPath.string());
        return;
    }

    m_ObjLoadResult = resource::LoadLitMesh(m_ModelPath);
    if (!m_ObjLoadResult.Ok()) {
        RequestStartupExit(1, m_ObjLoadResult.error);
        return;
    }

    m_Model = math::RotateY(math::kPi / 5.0f) * math::RotateX(-math::kPi / 7.0f);
    m_SourceTriangleCount = m_ObjLoadResult.mesh.Indices().size() / 3;
    m_SceneParts = {
        RenderScenePart{
            &m_ObjLoadResult.mesh,
            render::Color{ 255, 255, 255 },
            &m_ObjLoadResult.baseColorTexture,
        }
    };

    m_SceneView.parts = m_SceneParts.data();
    m_SceneView.partCount = m_SceneParts.size();
    m_SceneView.model = &m_Model;
    m_SceneView.backgroundColor = m_BackgroundColor;

    std::ostringstream startupMessage;
    startupMessage << "SoftRenderer OBJ textured viewer started on "
        << platform::Platform::Name()
        << " with scene \"" << m_ModelPath.string() << "\""
        << " ("
        << m_ObjLoadResult.mesh.Vertices().size() << " expanded vertices, "
        << m_SourceTriangleCount << " triangles, material \""
        << m_ObjLoadResult.materialName << "\", texture \""
        << m_ObjLoadResult.baseColorTexturePath.string() << "\" ("
        << m_ObjLoadResult.baseColorTexture.Width() << "x" << m_ObjLoadResult.baseColorTexture.Height() << "))"
        << '\n'
        << "Controls: WASD move, Space/Left Shift move vertically, Q/E turn\n";
    m_StartupMessage = startupMessage.str();
}

bool Application::InitializePlatform()
{
    if (platform::Platform::Initialize(m_Title, m_Width, m_Height)) {
        return true;
    }

    std::cerr << "Failed to create main window on platform: "
              << platform::Platform::Name() << '\n';
    platform::Platform::Close();
    return false;
}

int Application::RunMainLoop()
{
    auto previousFrameTime = std::chrono::steady_clock::now();
    int fpsFrameCount = 0;
    float fpsAccumulatedSeconds = 0.0f;
    bool wroteStatusLine = false;

    while (platform::Platform::ProcessEvents()) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
        previousFrameTime = currentFrameTime;

        m_LayerContext.activeCamera = &kFallbackCamera;
        m_LayerStack.OnUpdate(deltaTime, m_LayerContext);
        m_LayerStack.OnRender(m_LayerContext);

        if (!platform::Platform::Present(m_Framebuffer)) {
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

void Application::PrintStartupMessage() const
{
    std::cout << m_StartupMessage;
}

void Application::Shutdown()
{
    platform::Platform::Close();
    m_LayerStack.OnDetach(m_LayerContext);
}

void Application::RequestStartupExit(int exitCode, std::string message)
{
    m_ShouldExitOnStartup = true;
    m_StartupExitCode = exitCode;
    m_StartupExitMessage = std::move(message);
}

} // namespace sr
