#pragma once

#include "app/layer/Layer.h"
#include "app/layer/LayerContext.h"

#include "base/Math.h"
#include "render/EnvironmentMap.h"
#include "resource/loaders/ObjMeshLoader.h"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace sr {

class SceneLayer : public Layer {
public:
    struct IblPresetResources {
        const char* name = "Unknown";
        render::EnvironmentMap skybox;
        render::EnvironmentMap irradiance;
        render::PrefilterEnvironmentMap prefilter;
    };

    SceneLayer();

    void OnAttach(LayerContext& context) override;
    void OnUpdate(float deltaTime, LayerContext& context) override;
    void OnDetach(LayerContext& context) override;

private:
    void ActivateIblPreset(std::size_t index);

    render::Color m_BackgroundColor{ 18, 24, 38 };
    math::Mat4 m_Model = math::Mat4::Identity();
    std::filesystem::path m_ModelPath;
    resource::ObjLitLoadResult m_ObjLoadResult;
    render::Texture2D m_ProceduralAlbedoTexture;
    render::Texture2D m_ProceduralMetallicTexture;
    render::Texture2D m_ProceduralRoughnessTexture;
    render::Texture2D m_ProceduralAoTexture;
    render::Texture2D m_BrdfLut;
    std::vector<IblPresetResources> m_IblPresets;
    std::size_t m_ActiveIblPresetIndex = 0;
    bool m_PreviousPresetSwitchKeyDown = false;
    RenderSceneIblResources m_SceneIblResources;
    std::vector<RenderScenePart> m_SceneParts;
    RenderSceneView m_SceneView;
    std::size_t m_SourceTriangleCount = 0;
};

} // namespace sr
