#include "app/layer/RenderLayer.h"

#include "render/Camera.h"
#include "render/Pipeline.h"
#include "render/Program.h"
#include "render/VertexTypes.h"
#include "render/shader/LitShader.h"

namespace sr {
namespace {

void RenderLitMeshPart(render::Framebuffer& framebuffer,
    const render::Mesh<render::LitVertex>& mesh,
    const render::Color& color,
    const render::Texture2D* texture,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos)
{
    render::Program<render::LitVertex, render::LitUniforms, render::LitVaryings> program{};
    program.vertexShader = &render::LitVertexShader;
    program.fragmentShader = &render::LitFragmentShader;
    program.faceCullMode = render::FaceCullMode::Back;

    render::LitUniforms uniforms{};
    uniforms.mvp = projection * view * model;
    uniforms.model = model;
    uniforms.color = color;
    uniforms.lightPosWorld = lightPos;
    uniforms.cameraPosWorld = cameraPos;
    uniforms.baseColorTexture = texture;
    uniforms.textureEnabled = texture != nullptr ? 1.0f : 0.0f;

    render::Pipeline<render::LitVertex, render::LitUniforms, render::LitVaryings> pipeline;
    pipeline.Run(framebuffer, mesh, program, uniforms);
}

bool ReadyForRendering(const LayerContext& context)
{
    return context.activeCamera != nullptr
        && context.sceneView != nullptr
        && context.sceneView->Ready()
        && context.framebuffer != nullptr;
}

} // namespace

RenderLayer::RenderLayer()
    : Layer("RenderLayer")
{
}

void RenderLayer::OnRender(LayerContext& context)
{
    if (!ReadyForRendering(context)) {
        return;
    }

    const render::Camera& activeCamera = *context.activeCamera;
    const RenderSceneView& sceneView = *context.sceneView;
    render::Framebuffer& framebuffer = *context.framebuffer;
    const math::Vec3 lightPos{ 1.6f, 2.0f, 2.8f };

    framebuffer.Clear(sceneView.backgroundColor);
    framebuffer.ClearDepth();
    for (std::size_t index = 0; index < sceneView.partCount; ++index) {
        const RenderScenePart& part = sceneView.parts[index];
        if (part.mesh == nullptr) {
            continue;
        }

        RenderLitMeshPart(
            framebuffer,
            *part.mesh,
            part.color,
            part.texture,
            activeCamera.ProjectionMat4(),
            activeCamera.ViewMat4(),
            *sceneView.model,
            activeCamera.Pos,
            lightPos);
    }
}

} // namespace sr

