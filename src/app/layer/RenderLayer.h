#pragma once

#include "app/layer/LayerContext.h"
#include "app/layer/Layer.h"

namespace sr {

class RenderLayer : public Layer {
public:
    RenderLayer();

    void OnRender(LayerContext& context) override;
};

} // namespace sr

