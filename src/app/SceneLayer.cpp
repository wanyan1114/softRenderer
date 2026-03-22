#include "app/SceneLayer.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>

#ifndef SR_PROJECT_ROOT
#define SR_PROJECT_ROOT "."
#endif

namespace sr {
namespace {

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

void RequestStartupExit(LayerContext& context, int exitCode, std::string message)
{
    if (context.startupState == nullptr) {
        return;
    }

    context.startupState->shouldExit = true;
    context.startupState->exitCode = exitCode;
    context.startupState->exitMessage = std::move(message);
}

} // namespace

SceneLayer::SceneLayer()
    : Layer("SceneLayer")
{
}

void SceneLayer::OnAttach(LayerContext& context)
{
    m_ModelPath = GetDefaultModelPath();
    if (!IsObjPath(m_ModelPath)) {
        RequestStartupExit(context, 1, "Only OBJ rendering is supported in the current version: " + m_ModelPath.string());
        return;
    }

    m_ObjLoadResult = resource::LoadLitMesh(m_ModelPath);
    if (!m_ObjLoadResult.Ok()) {
        RequestStartupExit(context, 1, m_ObjLoadResult.error);
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
    context.sceneView = &m_SceneView;

    if (context.startupState != nullptr) {
        std::ostringstream startupMessage;
        startupMessage << "SoftRenderer OBJ textured viewer started on "
            << context.platformName
            << " with scene \"" << m_ModelPath.string() << "\""
            << " ("
            << m_ObjLoadResult.mesh.Vertices().size() << " expanded vertices, "
            << m_SourceTriangleCount << " triangles, material \""
            << m_ObjLoadResult.materialName << "\", texture \""
            << m_ObjLoadResult.baseColorTexturePath.string() << "\" ("
            << m_ObjLoadResult.baseColorTexture.Width() << "x" << m_ObjLoadResult.baseColorTexture.Height() << "))"
            << '\n'
            << "Controls: WASD move, Space/Left Shift move vertically, Q/E turn\n";
        context.startupState->startupMessage = startupMessage.str();
    }
}

void SceneLayer::OnDetach(LayerContext& context)
{
    if (context.sceneView == &m_SceneView) {
        context.sceneView = nullptr;
    }
}

} // namespace sr
