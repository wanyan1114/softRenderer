#pragma once

#include "app/LayerContext.h"
#include "app/Layer.h"

#include "render/Camera.h"

namespace sr {

class CameraLayer : public Layer {
public:
    CameraLayer(int viewportWidth, int viewportHeight);

    void OnAttach(LayerContext& context) override;
    void OnUpdate(float deltaTime, LayerContext& context) override;

private:
    int m_ViewportWidth = 1;
    int m_ViewportHeight = 1;
    render::Camera m_Camera{};
    float m_MoveSpeed = 2.0f;
    float m_RotateSpeed = math::kPi * 0.9f;
};

} // namespace sr
