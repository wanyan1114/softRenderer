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

    const auto makeProgram = [](render::FaceCullMode faceCullMode) {
        render::Program<render::FlatColorVertex,
            render::FlatColorUniforms,
            render::FlatColorVaryings> program{};
        program.vertexShader = &render::FlatColorVertexShader;
        program.fragmentShader = &render::FlatColorFragmentShader;
        program.faceCullMode = faceCullMode;
        return program;
    };

    const auto makeUniforms = [&](const render::Color& color) {
        render::FlatColorUniforms uniforms;
        uniforms.mvp = projection * view * model;
        uniforms.color = color;
        return uniforms;
    };

    const auto makePipeline = [&](const render::Color& color,
                                  render::FaceCullMode faceCullMode = render::FaceCullMode::Back) {
        return render::Pipeline<render::FlatColorVertex,
            render::FlatColorUniforms,
            render::FlatColorVaryings>(
                makeProgram(faceCullMode),
                makeUniforms(color));
    };

    auto clippedPipeline = makePipeline(render::Color{ 58, 134, 255 });
    auto overlapBackPipeline = makePipeline(render::Color{ 80, 200, 120 });
    auto overlapFrontPipeline = makePipeline(render::Color{ 255, 119, 71 });
    auto culledPipeline = makePipeline(render::Color{ 221, 103, 255 });
    auto doubleSidedPipeline = makePipeline(render::Color{ 255, 210, 74 }, render::FaceCullMode::None);

    const render::Mesh<render::FlatColorVertex> clippedMesh(
        {
            render::FlatColorVertex{ math::Vec3{ -2.60f, -0.46f, -0.10f } },
            render::FlatColorVertex{ math::Vec3{ -1.10f, 0.74f, -0.10f } },
            render::FlatColorVertex{ math::Vec3{ 0.10f, -0.28f, -0.10f } },
        },
        { 0, 2, 1 });
    const render::Mesh<render::FlatColorVertex> overlapBackMesh(
        {
            render::FlatColorVertex{ math::Vec3{ -0.48f, -0.62f, -0.28f } },
            render::FlatColorVertex{ math::Vec3{ -0.02f, 0.18f, -0.28f } },
            render::FlatColorVertex{ math::Vec3{ 0.42f, -0.56f, -0.28f } },
        },
        { 0, 2, 1 });
    const render::Mesh<render::FlatColorVertex> overlapFrontMesh(
        {
            render::FlatColorVertex{ math::Vec3{ -0.36f, -0.34f, 0.12f } },
            render::FlatColorVertex{ math::Vec3{ -0.02f, 0.54f, 0.12f } },
            render::FlatColorVertex{ math::Vec3{ 0.32f, -0.18f, 0.12f } },
        },
        { 0, 2, 1 });
    const render::Mesh<render::FlatColorVertex> culledMesh(
        {
            render::FlatColorVertex{ math::Vec3{ 0.28f, -0.46f, 0.02f } },
            render::FlatColorVertex{ math::Vec3{ 0.88f, -0.24f, 0.02f } },
            render::FlatColorVertex{ math::Vec3{ 0.56f, 0.52f, 0.02f } },
        },
        { 0, 2, 1 });
    const render::Mesh<render::FlatColorVertex> doubleSidedMesh(
        {
            render::FlatColorVertex{ math::Vec3{ 0.18f, 0.10f, -0.06f } },
            render::FlatColorVertex{ math::Vec3{ 0.86f, 0.18f, -0.06f } },
            render::FlatColorVertex{ math::Vec3{ 0.48f, 0.72f, -0.06f } },
        },
        { 0, 2, 1 });

    std::cout << "SoftRenderer clipping/culling scene started on "
              << platform::Platform::Name() << " with window \"" << m_Title
              << "\" (" << m_Width << "x" << m_Height << ")\n";

    platform::Platform::Show();

    while (platform::Platform::ProcessEvents()) {
        framebuffer.Clear(render::Color{ 18, 24, 38 });
        framebuffer.ClearDepth();

        clippedPipeline.Run(framebuffer, clippedMesh);
        overlapBackPipeline.Run(framebuffer, overlapBackMesh);
        overlapFrontPipeline.Run(framebuffer, overlapFrontMesh);
        culledPipeline.Run(framebuffer, culledMesh);
        doubleSidedPipeline.Run(framebuffer, doubleSidedMesh);

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
