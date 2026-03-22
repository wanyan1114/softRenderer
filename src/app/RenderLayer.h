#pragma once

#include "app/LayerContext.h"
#include "app/Layer.h"

namespace sr {

class RenderLayer : public Layer {
public:
    RenderLayer();

    void OnRender(LayerContext& context) override;
};

} // namespace sr
