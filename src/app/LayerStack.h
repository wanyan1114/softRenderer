#pragma once

#include "app/Layer.h"

#include <memory>
#include <utility>
#include <vector>

namespace sr {

class LayerStack {
public:
    LayerStack() = default;

    ~LayerStack() = default;

    template<typename layer_t, typename... args_t>
    layer_t* EmplaceLayer(args_t&&... args)
    {
        auto layer = std::make_unique<layer_t>(std::forward<args_t>(args)...);
        layer_t* rawLayer = layer.get();
        m_Layers.push_back(std::move(layer));
        return rawLayer;
    }

    void OnAttach(LayerContext& context)
    {
        if (m_Attached) {
            return;
        }

        for (const std::unique_ptr<Layer>& layer : m_Layers) {
            layer->OnAttach(context);
        }
        m_Attached = true;
    }

    void OnDetach(LayerContext& context)
    {
        if (!m_Attached) {
            return;
        }

        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it) {
            (*it)->OnDetach(context);
        }
        m_Attached = false;
    }

    void OnUpdate(float deltaTime, LayerContext& context)
    {
        for (const std::unique_ptr<Layer>& layer : m_Layers) {
            layer->OnUpdate(deltaTime, context);
        }
    }

    void OnRender(LayerContext& context)
    {
        for (const std::unique_ptr<Layer>& layer : m_Layers) {
            layer->OnRender(context);
        }
    }

private:
    std::vector<std::unique_ptr<Layer>> m_Layers;
    bool m_Attached = false;
};

} // namespace sr
