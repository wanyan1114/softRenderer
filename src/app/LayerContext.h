#pragma once

#include "base/Math.h"
#include "render/Color.h"
#include "render/Mesh.h"
#include "render/Texture2D.h"
#include "render/Vertex.h"

#include <cstddef>

namespace sr::render {
class Camera;
class Framebuffer;
}

namespace sr {

struct RenderScenePart {
    const render::Mesh<render::LitVertex>* mesh = nullptr;
    render::Color color{ 255, 255, 255 };
    const render::Texture2D* texture = nullptr;
};

struct RenderSceneView {
    const RenderScenePart* parts = nullptr;
    std::size_t partCount = 0;
    const math::Mat4* model = nullptr;
    render::Color backgroundColor{ 18, 24, 38 };

    bool Ready() const
    {
        return parts != nullptr && partCount > 0 && model != nullptr;
    }
};

struct LayerContext {
    const render::Camera* activeCamera = nullptr;
    const RenderSceneView* sceneView = nullptr;
    render::Framebuffer* framebuffer = nullptr;
};

} // namespace sr
