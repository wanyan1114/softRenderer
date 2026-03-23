#pragma once

#include "base/Math.h"
#include "platform/Input.h"
#include "render/Color.h"
#include "render/Mesh.h"
#include "render/Texture2D.h"
#include "render/VertexTypes.h"

#include <cstddef>
#include <string>

namespace sr::render {
class Camera;
class EnvironmentMap;
class Framebuffer;
class PrefilterEnvironmentMap;
}

namespace sr {

struct RenderScenePart {
    const render::Mesh<render::LitVertex>* mesh = nullptr;
    render::Color color{ 255, 255, 255 };
    const render::Texture2D* albedoTexture = nullptr;
    const render::Texture2D* metallicTexture = nullptr;
    const render::Texture2D* roughnessTexture = nullptr;
    const render::Texture2D* aoTexture = nullptr;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float aoFactor = 1.0f;
};

struct RenderSceneIblResources {
    const render::EnvironmentMap* skybox = nullptr;
    const render::EnvironmentMap* irradiance = nullptr;
    const render::PrefilterEnvironmentMap* prefilter = nullptr;
    const render::Texture2D* brdfLut = nullptr;
    const char* presetName = "None";

    bool Ready() const
    {
        return skybox != nullptr
            && irradiance != nullptr
            && prefilter != nullptr
            && brdfLut != nullptr;
    }
};

struct RenderSceneView {
    const RenderScenePart* parts = nullptr;
    std::size_t partCount = 0;
    const math::Mat4* model = nullptr;
    const RenderSceneIblResources* ibl = nullptr;
    render::Color backgroundColor{ 18, 24, 38 };

    bool Ready() const
    {
        return parts != nullptr && partCount > 0 && model != nullptr;
    }
};

struct StartupState {
    bool shouldExit = false;
    int exitCode = 0;
    std::string exitMessage;
    std::string startupMessage;
};

struct LayerContext {
    const render::Camera* activeCamera = nullptr;
    const RenderSceneView* sceneView = nullptr;
    render::Framebuffer* framebuffer = nullptr;
    const platform::InputState* input = nullptr;
    StartupState* startupState = nullptr;
    const char* platformName = "Unknown";
};

} // namespace sr
