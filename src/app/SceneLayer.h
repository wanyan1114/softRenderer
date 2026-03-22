#pragma once

#include "app/LayerContext.h"
#include "app/Layer.h"

#include "base/Math.h"
#include "resource/loaders/ObjMeshLoader.h"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace sr {

class SceneLayer : public Layer {
public:
    SceneLayer();

    void OnAttach(LayerContext& context) override;
    void OnDetach(LayerContext& context) override;

private:
    render::Color m_BackgroundColor{ 18, 24, 38 };
    math::Mat4 m_Model = math::Mat4::Identity();
    std::filesystem::path m_ModelPath;
    resource::ObjLitLoadResult m_ObjLoadResult;
    std::vector<RenderScenePart> m_SceneParts;
    RenderSceneView m_SceneView;
    std::size_t m_SourceTriangleCount = 0;
};

} // namespace sr
