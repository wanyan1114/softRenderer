#include "app/layer/SceneLayer.h"

#include "platform/Input.h"
#include "render/IblPrecompute.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef SR_PROJECT_ROOT
#define SR_PROJECT_ROOT "."
#endif

namespace sr {
namespace {

constexpr int kProceduralTextureSize = 256;

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

bool IsKeyDown(const LayerContext& context, platform::Key key)
{
    return context.input != nullptr && context.input->IsKeyDown(key);
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

render::Texture2D BuildProceduralAlbedoTexture(int width, int height)
{
    std::vector<render::Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const float v = 1.0f - static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
            const int checkerX = static_cast<int>(u * 6.0f);
            const int checkerY = static_cast<int>(v * 6.0f);
            const bool evenChecker = ((checkerX + checkerY) % 2) == 0;
            const float checkerScale = evenChecker ? 1.0f : 0.72f;

            float red = math::Lerp(0.42f, 0.95f, u) * checkerScale;
            float green = math::Lerp(0.18f, 0.78f, v) * checkerScale;
            float blue = math::Lerp(0.12f, 0.34f, 1.0f - u * 0.65f) * checkerScale;

            const bool accentBand = std::abs(u - 0.5f) < 0.05f || std::abs(v - 0.5f) < 0.05f;
            if (accentBand) {
                red = math::Lerp(red, 0.96f, 0.35f);
                green = math::Lerp(green, 0.82f, 0.35f);
                blue = math::Lerp(blue, 0.22f, 0.35f);
            }

            const bool border = u < 0.03f || u > 0.97f || v < 0.03f || v > 0.97f;
            if (border) {
                red *= 0.18f;
                green *= 0.18f;
                blue *= 0.18f;
            }

            pixels.push_back(render::Color::FromFloats(red, green, blue));
        }
    }

    return render::Texture2D(width, height, std::move(pixels));
}

render::Texture2D BuildProceduralMetallicTexture(int width, int height)
{
    std::vector<render::Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const float v = 1.0f - static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));

            float metallic = 0.04f;
            if (u > 0.33f && u <= 0.66f) {
                metallic = 0.55f;
            } else if (u > 0.66f) {
                metallic = 1.0f;
            }

            if (v > 0.72f) {
                metallic = math::Clamp(metallic * 0.75f + 0.12f, 0.0f, 1.0f);
            }

            pixels.push_back(render::Color::FromFloats(metallic, metallic, metallic));
        }
    }

    return render::Texture2D(width, height, std::move(pixels));
}

render::Texture2D BuildProceduralRoughnessTexture(int width, int height)
{
    std::vector<render::Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const float v = 1.0f - static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));

            float roughness = math::Lerp(0.08f, 0.92f, u);
            if (v > 0.6f) {
                roughness = math::Clamp(roughness * 0.75f + 0.08f, 0.045f, 1.0f);
            }

            const bool guideBand = std::abs(v - 0.5f) < 0.03f;
            if (guideBand) {
                roughness = math::Clamp(roughness + 0.08f, 0.045f, 1.0f);
            }

            pixels.push_back(render::Color::FromFloats(roughness, roughness, roughness));
        }
    }

    return render::Texture2D(width, height, std::move(pixels));
}

render::Texture2D BuildProceduralAoTexture(int width, int height)
{
    std::vector<render::Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        const float v = 1.0f - static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
            const float dx = std::abs(u - 0.5f) * 2.0f;
            const float dy = std::abs(v - 0.5f) * 2.0f;
            const float vignette = std::max(dx, dy);
            float ao = 1.0f - 0.58f * std::pow(math::Clamp(vignette, 0.0f, 1.0f), 1.35f);

            const bool crease = std::abs(u - 0.5f) < 0.035f || std::abs(v - 0.5f) < 0.035f;
            if (crease) {
                ao *= 0.78f;
            }

            ao = math::Clamp(ao, 0.28f, 1.0f);
            pixels.push_back(render::Color::FromFloats(ao, ao, ao));
        }
    }

    return render::Texture2D(width, height, std::move(pixels));
}

SceneLayer::IblPresetResources BuildIblPresetResources(render::ibl::EnvironmentPreset preset)
{
    SceneLayer::IblPresetResources resources{};
    resources.name = render::ibl::EnvironmentPresetName(preset);
    resources.skybox = render::ibl::GenerateEnvironmentPreset(preset);
    resources.irradiance = render::ibl::ConvoluteDiffuseIrradiance(resources.skybox);
    resources.prefilter = render::ibl::PrefilterEnvironment(resources.skybox);
    return resources;
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

    m_ProceduralAlbedoTexture = BuildProceduralAlbedoTexture(kProceduralTextureSize, kProceduralTextureSize);
    m_ProceduralMetallicTexture = BuildProceduralMetallicTexture(kProceduralTextureSize, kProceduralTextureSize);
    m_ProceduralRoughnessTexture = BuildProceduralRoughnessTexture(kProceduralTextureSize, kProceduralTextureSize);
    m_ProceduralAoTexture = BuildProceduralAoTexture(kProceduralTextureSize, kProceduralTextureSize);
    m_BrdfLut = render::ibl::IntegrateBrdfLut();

    m_IblPresets.clear();
    for (const render::ibl::EnvironmentPreset preset : render::ibl::AllEnvironmentPresets()) {
        m_IblPresets.push_back(BuildIblPresetResources(preset));
    }
    ActivateIblPreset(0);

    m_Model = math::RotateY(math::kPi / 5.0f) * math::RotateX(-math::kPi / 7.0f);
    m_SourceTriangleCount = m_ObjLoadResult.mesh.Indices().size() / 3;
    m_SceneParts = {
        RenderScenePart{
            &m_ObjLoadResult.mesh,
            render::Color{ 255, 255, 255 },
            &m_ProceduralAlbedoTexture,
            &m_ProceduralMetallicTexture,
            &m_ProceduralRoughnessTexture,
            &m_ProceduralAoTexture,
            1.0f,
            1.0f,
            1.0f,
        }
    };

    m_SceneView.parts = m_SceneParts.data();
    m_SceneView.partCount = m_SceneParts.size();
    m_SceneView.model = &m_Model;
    m_SceneView.ibl = &m_SceneIblResources;
    m_SceneView.backgroundColor = m_BackgroundColor;
    context.sceneView = &m_SceneView;

    if (context.startupState != nullptr) {
        std::ostringstream startupMessage;
        startupMessage << "SoftRenderer OBJ viewer started on "
            << context.platformName
            << " with scene \"" << m_ModelPath.string() << "\""
            << " ("
            << m_ObjLoadResult.mesh.Vertices().size() << " expanded vertices, "
            << m_SourceTriangleCount << " triangles, source material \""
            << m_ObjLoadResult.materialName << "\")"
            << '\n'
            << "Procedural material textures: albedo / metallic / roughness / ao ("
            << kProceduralTextureSize << "x" << kProceduralTextureSize << ")"
            << '\n'
            << "Generated IBL presets: " << m_IblPresets.size()
            << ", active preset: " << m_SceneIblResources.presetName
            << ", BRDF LUT: " << m_BrdfLut.Width() << "x" << m_BrdfLut.Height()
            << '\n'
            << "Controls: WASD move, Space/Left Shift move vertically, Q/E turn"
            << '\n'
            << "F cycle render mode, G cycle skybox view, T cycle prefilter preview lod, R cycle environment preset"
            << '\n';
        context.startupState->startupMessage = startupMessage.str();
    }
}

void SceneLayer::OnUpdate(float, LayerContext& context)
{
    const bool presetSwitchDown = IsKeyDown(context, platform::Key::R);
    if (presetSwitchDown && !m_PreviousPresetSwitchKeyDown && !m_IblPresets.empty()) {
        const std::size_t nextIndex = (m_ActiveIblPresetIndex + 1) % m_IblPresets.size();
        ActivateIblPreset(nextIndex);
        std::cout << "\nEnvironment preset switched to " << m_SceneIblResources.presetName << "\n";
    }

    m_PreviousPresetSwitchKeyDown = presetSwitchDown;
    context.sceneView = &m_SceneView;
}

void SceneLayer::OnDetach(LayerContext& context)
{
    if (context.sceneView == &m_SceneView) {
        context.sceneView = nullptr;
    }
}

void SceneLayer::ActivateIblPreset(std::size_t index)
{
    if (m_IblPresets.empty()) {
        m_ActiveIblPresetIndex = 0;
        m_SceneIblResources = {};
        return;
    }

    m_ActiveIblPresetIndex = std::min(index, m_IblPresets.size() - 1);
    const IblPresetResources& preset = m_IblPresets[m_ActiveIblPresetIndex];
    m_SceneIblResources.skybox = &preset.skybox;
    m_SceneIblResources.irradiance = &preset.irradiance;
    m_SceneIblResources.prefilter = &preset.prefilter;
    m_SceneIblResources.brdfLut = &m_BrdfLut;
    m_SceneIblResources.presetName = preset.name;
}

} // namespace sr
