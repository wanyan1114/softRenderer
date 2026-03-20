#pragma once

#include "render/Camera.h"
#include "render/Color.h"

namespace sr::render {
class Framebuffer;
}

namespace sr {

class CameraLayer {
public:
    CameraLayer();
    ~CameraLayer();

    CameraLayer(const CameraLayer&) = delete;
    CameraLayer& operator=(const CameraLayer&) = delete;

    static CameraLayer& Get();

    void OnAttach(const render::Framebuffer& framebuffer);
    void OnUpdate(float deltaTime);

    const render::Camera& Camera() const { return m_Camera; }
    render::Camera& Camera() { return m_Camera; }
    const render::Color& BackgroundColor() const { return m_BackgroundColor; }

private:
    static CameraLayer* s_Instance;

    render::Color m_BackgroundColor{ 18, 24, 38 };
    render::Camera m_Camera{};
    float m_MoveSpeed = 2.0f;
    float m_RotateSpeed = math::kPi * 0.9f;
};

} // namespace sr
