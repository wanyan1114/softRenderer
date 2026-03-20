#include "app/CameraLayer.h"

#include "platform/Input.h"
#include "platform/Platform.h"
#include "render/Framebuffer.h"

namespace sr {

CameraLayer* CameraLayer::s_Instance = nullptr;

namespace {

math::Vec3 WorldUp()
{
    return { 0.0f, 1.0f, 0.0f };
}

} // namespace

CameraLayer::CameraLayer()
{
    s_Instance = this;
}

CameraLayer::~CameraLayer()
{
    if (s_Instance == this) {
        s_Instance = nullptr;
    }
}

CameraLayer& CameraLayer::Get()
{
    return *s_Instance;
}

void CameraLayer::OnAttach(const render::Framebuffer& framebuffer)
{
    const int width = framebuffer.Width();
    const int height = framebuffer.Height();
    m_Camera.Aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    m_Camera.Pos = { 0.0f, 0.0f, 2.6f };
    m_Camera.Right = { 1.0f, 0.0f, 0.0f };
    m_Camera.Up = WorldUp();
    m_Camera.Dir = { 0.0f, 0.0f, -1.0f };
}

void CameraLayer::OnUpdate(float deltaTime)
{
    const float clampedDeltaTime = math::Clamp(deltaTime, 0.0f, 0.1f);
    const float yawDelta = ((platform::Platform::IsKeyDown(platform::Key::Q) ? 1.0f : 0.0f)
                               - (platform::Platform::IsKeyDown(platform::Key::E) ? 1.0f : 0.0f))
        * m_RotateSpeed * clampedDeltaTime;

    if (yawDelta != 0.0f) {
        const math::Vec4 rotatedDir = math::RotateY(yawDelta) * math::Vec4(m_Camera.Dir, 0.0f);
        const math::Vec4 rotatedRight = math::RotateY(yawDelta) * math::Vec4(m_Camera.Right, 0.0f);
        m_Camera.Dir = math::Vec3{ rotatedDir.x, rotatedDir.y, rotatedDir.z }.Normalized();
        m_Camera.Right = math::Vec3{ rotatedRight.x, rotatedRight.y, rotatedRight.z }.Normalized();
    }

    math::Vec3 movement{};
    if (platform::Platform::IsKeyDown(platform::Key::W)) {
        movement = movement + m_Camera.Dir;
    }
    if (platform::Platform::IsKeyDown(platform::Key::S)) {
        movement = movement - m_Camera.Dir;
    }
    if (platform::Platform::IsKeyDown(platform::Key::D)) {
        movement = movement + m_Camera.Right;
    }
    if (platform::Platform::IsKeyDown(platform::Key::A)) {
        movement = movement - m_Camera.Right;
    }
    if (platform::Platform::IsKeyDown(platform::Key::Space)) {
        movement = movement + m_Camera.Up;
    }
    if (platform::Platform::IsKeyDown(platform::Key::LeftShift)) {
        movement = movement - m_Camera.Up;
    }

    if (movement.Length() > 0.0f) {
        m_Camera.Pos = m_Camera.Pos + movement.Normalized() * (m_MoveSpeed * clampedDeltaTime);
    }
}

} // namespace sr
