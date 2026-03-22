#pragma once

#include "app/LayerStack.h"
#include "app/LayerContext.h"

#include "base/Math.h"
#include "render/Color.h"
#include "render/Framebuffer.h"
#include "resource/loaders/ObjMeshLoader.h"

#include <filesystem>
#include <string>
#include <vector>

namespace sr {

class Application {
public:
    Application(std::string title, int width, int height);

    int Run();

private:
    void BuildLayers();
    void PrepareScene();
    bool InitializePlatform();
    int RunMainLoop();
    void PrintStartupMessage() const;
    void Shutdown();
    void RequestStartupExit(int exitCode, std::string message);

    std::string m_Title;
    int m_Width;
    int m_Height;
    LayerStack m_LayerStack;
    render::Framebuffer m_Framebuffer;
    LayerContext m_LayerContext;
    render::Color m_BackgroundColor{ 18, 24, 38 };
    math::Mat4 m_Model = math::Mat4::Identity();
    std::filesystem::path m_ModelPath;
    resource::ObjLitLoadResult m_ObjLoadResult;
    std::vector<RenderScenePart> m_SceneParts;
    RenderSceneView m_SceneView;
    std::size_t m_SourceTriangleCount = 0;
    bool m_ShouldExitOnStartup = false;
    int m_StartupExitCode = 0;
    std::string m_StartupExitMessage;
    std::string m_StartupMessage;
};

} // namespace sr
