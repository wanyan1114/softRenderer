#include "app/layer/CameraLayer.h"

#include "platform/Input.h"

namespace sr {
namespace {

math::Vec3 WorldUp()
{
    return { 0.0f, 1.0f, 0.0f };
}

bool IsKeyDown(const LayerContext& context, platform::Key key)
{
    return context.input != nullptr && context.input->IsKeyDown(key);
}

} // namespace

CameraLayer::CameraLayer(int viewportWidth, int viewportHeight)
    : Layer("CameraLayer"),
      m_ViewportWidth(viewportWidth),
      m_ViewportHeight(viewportHeight)
{
}

void CameraLayer::OnAttach(LayerContext& context)
{
    const int safeWidth = m_ViewportWidth > 0 ? m_ViewportWidth : 1;
    const int safeHeight = m_ViewportHeight > 0 ? m_ViewportHeight : 1;

    m_Camera.Aspect = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
    m_Camera.Pos = { 0.0f, 0.0f, 2.6f };
    m_Camera.Right = { 1.0f, 0.0f, 0.0f };
    m_Camera.Up = WorldUp();
    m_Camera.Dir = { 0.0f, 0.0f, -1.0f };
    context.activeCamera = &m_Camera;
}

void CameraLayer::OnUpdate(float deltaTime, LayerContext& context)
{
    const float clampedDeltaTime = math::Clamp(deltaTime, 0.0f, 0.1f);
    const float yawDelta = ((IsKeyDown(context, platform::Key::Q) ? 1.0f : 0.0f)
                               - (IsKeyDown(context, platform::Key::E) ? 1.0f : 0.0f))
        * m_RotateSpeed * clampedDeltaTime;

    if (yawDelta != 0.0f) {
        const math::Vec4 rotatedDir = math::RotateY(yawDelta) * math::Vec4(m_Camera.Dir, 0.0f);
        const math::Vec4 rotatedRight = math::RotateY(yawDelta) * math::Vec4(m_Camera.Right, 0.0f);
        m_Camera.Dir = math::Vec3{ rotatedDir.x, rotatedDir.y, rotatedDir.z }.Normalized();
        m_Camera.Right = math::Vec3{ rotatedRight.x, rotatedRight.y, rotatedRight.z }.Normalized();
    }

    math::Vec3 movement{};
    if (IsKeyDown(context, platform::Key::W)) {
        movement = movement + m_Camera.Dir;
    }
    if (IsKeyDown(context, platform::Key::S)) {
        movement = movement - m_Camera.Dir;
    }
    if (IsKeyDown(context, platform::Key::D)) {
        movement = movement + m_Camera.Right;
    }
    if (IsKeyDown(context, platform::Key::A)) {
        movement = movement - m_Camera.Right;
    }
    if (IsKeyDown(context, platform::Key::Space)) {
        movement = movement + m_Camera.Up;
    }
    if (IsKeyDown(context, platform::Key::LeftShift)) {
        movement = movement - m_Camera.Up;
    }

    if (movement.Length() > 0.0f) {
        m_Camera.Pos = m_Camera.Pos + movement.Normalized() * (m_MoveSpeed * clampedDeltaTime);
    }

    context.activeCamera = &m_Camera;
}

} // namespace sr

