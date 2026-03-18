#include "app/Application.h"

#include "base/Math.h"
#include "platform/Platform.h"
#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/Pipeline.h"
#include "render/Vertex.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace sr {

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)), m_Width(width), m_Height(height)
{
}

int Application::Run()
{
    using namespace std::chrono_literals;

    if (!platform::Platform::Initialize(m_Title, m_Width, m_Height)) {
        std::cerr << "Failed to create main window on platform: "
                  << platform::Platform::Name() << '\n';
        platform::Platform::Close();
        return 1;
    }

    render::Framebuffer framebuffer(m_Width, m_Height);

    const math::Mat4 model = math::Mat4::Identity();
    const math::Mat4 view = math::LookAt(
        math::Vec3{ 0.0f, 0.0f, 2.2f },
        math::Vec3{ 0.0f, 0.0f, 1.0f },
        math::Vec3{ 0.0f, 1.0f, 0.0f });
    const math::Mat4 projection = math::Perspective(
        math::kPi / 3.0f,
        static_cast<float>(m_Width) / static_cast<float>(m_Height),
        0.1f,
        10.0f);

    const render::Program<render::FlatColorVertex,
        render::FlatColorUniforms,
        render::FlatColorVaryings> program{
        &render::FlatColorVertexShader,
        &render::FlatColorFragmentShader,
    };

    const auto makeUniforms = [&](const render::Color& color) {
        render::FlatColorUniforms uniforms;
        uniforms.mvp = projection * view * model;
        uniforms.color = color;
        return uniforms;
    };

    render::Pipeline<render::FlatColorVertex,
        render::FlatColorUniforms,
        render::FlatColorVaryings> leftBackPipeline(
            program,
            makeUniforms(render::Color{ 58, 134, 255 }));
    render::Pipeline<render::FlatColorVertex,
        render::FlatColorUniforms,
        render::FlatColorVaryings> leftFrontPipeline(
            program,
            makeUniforms(render::Color{ 255, 119, 71 }));
    render::Pipeline<render::FlatColorVertex,
        render::FlatColorUniforms,
        render::FlatColorVaryings> rightBackPipeline(
            program,
            makeUniforms(render::Color{ 80, 200, 120 }));
    render::Pipeline<render::FlatColorVertex,
        render::FlatColorUniforms,
        render::FlatColorVaryings> rightFrontPipeline(
            program,
            makeUniforms(render::Color{ 221, 103, 255 }));

    const render::Mesh<render::FlatColorVertex> leftBackMesh(
        {
            render::FlatColorVertex{ math::Vec3{ -0.90f, -0.66f, -0.30f } },
            render::FlatColorVertex{ math::Vec3{ -0.20f, -0.56f, -0.30f } },
            render::FlatColorVertex{ math::Vec3{ -0.58f, 0.24f, -0.30f } },
        },
        { 0, 1, 2 });
    const render::Mesh<render::FlatColorVertex> leftFrontMesh(
        {
            render::FlatColorVertex{ math::Vec3{ -0.78f, -0.36f, 0.10f } },
            render::FlatColorVertex{ math::Vec3{ -0.10f, -0.22f, 0.10f } },
            render::FlatColorVertex{ math::Vec3{ -0.42f, 0.56f, 0.10f } },
        },
        { 0, 1, 2 });
    const render::Mesh<render::FlatColorVertex> rightBackMesh(
        {
            render::FlatColorVertex{ math::Vec3{ 0.04f, -0.62f, -0.05f } },
            render::FlatColorVertex{ math::Vec3{ 0.84f, -0.68f, -0.05f } },
            render::FlatColorVertex{ math::Vec3{ 0.48f, 0.18f, -0.05f } },
        },
        { 0, 1, 2 });
    const render::Mesh<render::FlatColorVertex> rightFrontMesh(
        {
            render::FlatColorVertex{ math::Vec3{ 0.18f, -0.30f, 0.22f } },
            render::FlatColorVertex{ math::Vec3{ 0.92f, -0.18f, 0.22f } },
            render::FlatColorVertex{ math::Vec3{ 0.56f, 0.62f, 0.22f } },
        },
        { 0, 1, 2 });

    std::cout << "SoftRenderer overlap scene started on "
              << platform::Platform::Name() << " with window \"" << m_Title
              << "\" (" << m_Width << "x" << m_Height << ")\n";

    platform::Platform::Show();

    while (platform::Platform::ProcessEvents()) {
        framebuffer.Clear(render::Color{ 18, 24, 38 });
        framebuffer.ClearDepth();

        leftBackPipeline.Run(framebuffer, leftBackMesh);
        leftFrontPipeline.Run(framebuffer, leftFrontMesh);
        rightBackPipeline.Run(framebuffer, rightBackMesh);
        rightFrontPipeline.Run(framebuffer, rightFrontMesh);

        if (!platform::Platform::Present(framebuffer)) {
            std::cerr << "Failed to present framebuffer to window.\n";
            platform::Platform::Close();
            return 1;
        }

        std::this_thread::sleep_for(16ms);
    }

    platform::Platform::Close();
    return 0;
}

} // namespace sr
