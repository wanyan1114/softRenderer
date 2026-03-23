#pragma once

#include "app/layer/Layer.h"
#include "app/layer/LayerContext.h"
#include "render/SkyboxRenderer.h"

namespace sr {

class RenderLayer : public Layer {
public:
    enum class RenderMode {
        Blinn = 0,
        DirectPbr,
        IblPbr,
    };

    RenderLayer();

    void OnUpdate(float deltaTime, LayerContext& context) override;
    void OnRender(LayerContext& context) override;

private:
    RenderMode m_RenderMode = RenderMode::IblPbr;
    render::SkyboxDisplayMode m_SkyboxMode = render::SkyboxDisplayMode::Skybox;
    int m_PrefilterPreviewLod = 2;
    bool m_PreviousRenderModeKeyDown = false;
    bool m_PreviousSkyboxModeKeyDown = false;
    bool m_PreviousPrefilterLodKeyDown = false;
};

} // namespace sr
