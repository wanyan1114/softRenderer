#include "app/layer/RenderLayer.h"

#include "platform/Input.h"
#include "render/Camera.h"
#include "render/Pipeline.h"
#include "render/Program.h"
#include "render/VertexTypes.h"
#include "render/shader/IblPbrShader.h"
#include "render/shader/LitShader.h"
#include "render/shader/PbrShader.h"

#include <algorithm>
#include <iostream>

namespace sr {
namespace {

void RenderBlinnMeshPart(render::Framebuffer& framebuffer,
    const RenderScenePart& part,
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
    uniforms.color = part.color;
    uniforms.lightPosWorld = lightPos;
    uniforms.cameraPosWorld = cameraPos;
    uniforms.baseColorTexture = part.albedoTexture;
    uniforms.textureEnabled = part.albedoTexture != nullptr ? 1.0f : 0.0f;

    render::Pipeline<render::LitVertex, render::LitUniforms, render::LitVaryings> pipeline;
    pipeline.Run(framebuffer, *part.mesh, program, uniforms);
}

void RenderDirectPbrMeshPart(render::Framebuffer& framebuffer,
    const RenderScenePart& part,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos)
{
    render::Program<render::LitVertex, render::PbrUniforms, render::PbrVaryings> program{};
    program.vertexShader = &render::PbrVertexShader;
    program.fragmentShader = &render::PbrFragmentShader;
    program.faceCullMode = render::FaceCullMode::Back;

    render::PbrUniforms uniforms{};
    uniforms.mvp = projection * view * model;
    uniforms.model = model;
    uniforms.albedoColor = part.color;
    uniforms.lightPosWorld = lightPos;
    uniforms.cameraPosWorld = cameraPos;
    uniforms.albedoTexture = part.albedoTexture;
    uniforms.metallicTexture = part.metallicTexture;
    uniforms.roughnessTexture = part.roughnessTexture;
    uniforms.aoTexture = part.aoTexture;
    uniforms.metallicFactor = part.metallicFactor;
    uniforms.roughnessFactor = part.roughnessFactor;
    uniforms.aoFactor = part.aoFactor;

    render::Pipeline<render::LitVertex, render::PbrUniforms, render::PbrVaryings> pipeline;
    pipeline.Run(framebuffer, *part.mesh, program, uniforms);
}

void RenderIblPbrMeshPart(render::Framebuffer& framebuffer,
    const RenderScenePart& part,
    const RenderSceneIblResources& ibl,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos)
{
    render::Program<render::LitVertex, render::IblPbrUniforms, render::IblPbrVaryings> program{};
    program.vertexShader = &render::IblPbrVertexShader;
    program.fragmentShader = &render::IblPbrFragmentShader;
    program.faceCullMode = render::FaceCullMode::Back;

    render::IblPbrUniforms uniforms{};
    uniforms.mvp = projection * view * model;
    uniforms.model = model;
    uniforms.albedoColor = part.color;
    uniforms.lightPosWorld = lightPos;
    uniforms.cameraPosWorld = cameraPos;
    uniforms.albedoTexture = part.albedoTexture;
    uniforms.metallicTexture = part.metallicTexture;
    uniforms.roughnessTexture = part.roughnessTexture;
    uniforms.aoTexture = part.aoTexture;
    uniforms.irradianceMap = ibl.irradiance;
    uniforms.prefilterMap = ibl.prefilter;
    uniforms.brdfLut = ibl.brdfLut;
    uniforms.metallicFactor = part.metallicFactor;
    uniforms.roughnessFactor = part.roughnessFactor;
    uniforms.aoFactor = part.aoFactor;

    render::Pipeline<render::LitVertex, render::IblPbrUniforms, render::IblPbrVaryings> pipeline;
    pipeline.Run(framebuffer, *part.mesh, program, uniforms);
}

bool ReadyForRendering(const LayerContext& context)
{
    return context.activeCamera != nullptr
        && context.sceneView != nullptr
        && context.sceneView->Ready()
        && context.framebuffer != nullptr;
}

bool IsKeyDown(const LayerContext& context, platform::Key key)
{
    return context.input != nullptr && context.input->IsKeyDown(key);
}

const char* RenderModeName(RenderLayer::RenderMode mode)
{
    switch (mode) {
    case RenderLayer::RenderMode::Blinn:
        return "Blinn-Phong";
    case RenderLayer::RenderMode::DirectPbr:
        return "Direct PBR";
    case RenderLayer::RenderMode::IblPbr:
        return "IBL-PBR";
    }

    return "Unknown";
}

RenderLayer::RenderMode NextRenderMode(RenderLayer::RenderMode mode)
{
    switch (mode) {
    case RenderLayer::RenderMode::Blinn:
        return RenderLayer::RenderMode::DirectPbr;
    case RenderLayer::RenderMode::DirectPbr:
        return RenderLayer::RenderMode::IblPbr;
    case RenderLayer::RenderMode::IblPbr:
        return RenderLayer::RenderMode::Blinn;
    }

    return RenderLayer::RenderMode::IblPbr;
}

render::SkyboxDisplayMode NextSkyboxMode(render::SkyboxDisplayMode mode)
{
    switch (mode) {
    case render::SkyboxDisplayMode::Skybox:
        return render::SkyboxDisplayMode::Irradiance;
    case render::SkyboxDisplayMode::Irradiance:
        return render::SkyboxDisplayMode::Prefilter;
    case render::SkyboxDisplayMode::Prefilter:
        return render::SkyboxDisplayMode::Skybox;
    }

    return render::SkyboxDisplayMode::Skybox;
}

} // namespace

RenderLayer::RenderLayer()
    : Layer("RenderLayer")
{
}

void RenderLayer::OnUpdate(float, LayerContext& context)
{
    const bool renderModeDown = IsKeyDown(context, platform::Key::F);
    if (renderModeDown && !m_PreviousRenderModeKeyDown) {
        m_RenderMode = NextRenderMode(m_RenderMode);
        std::cout << "\nRender mode switched to " << RenderModeName(m_RenderMode) << "\n";
    }
    m_PreviousRenderModeKeyDown = renderModeDown;

    const bool skyboxModeDown = IsKeyDown(context, platform::Key::G);
    if (skyboxModeDown && !m_PreviousSkyboxModeKeyDown) {
        m_SkyboxMode = NextSkyboxMode(m_SkyboxMode);
        std::cout << "Skybox view switched to " << render::SkyboxDisplayModeName(m_SkyboxMode) << "\n";
    }
    m_PreviousSkyboxModeKeyDown = skyboxModeDown;

    const bool prefilterLodDown = IsKeyDown(context, platform::Key::T);
    if (prefilterLodDown && !m_PreviousPrefilterLodKeyDown) {
        const RenderSceneIblResources* ibl = context.sceneView != nullptr ? context.sceneView->ibl : nullptr;
        const int levelCount = (ibl != nullptr && ibl->prefilter != nullptr) ? ibl->prefilter->LevelCount() : 0;
        if (levelCount > 0) {
            m_PrefilterPreviewLod = (m_PrefilterPreviewLod + 1) % levelCount;
            std::cout << "Prefilter preview LOD switched to " << m_PrefilterPreviewLod << "\n";
        }
    }
    m_PreviousPrefilterLodKeyDown = prefilterLodDown;
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
    if (sceneView.ibl != nullptr && sceneView.ibl->Ready()) {
        switch (m_SkyboxMode) {
        case render::SkyboxDisplayMode::Skybox:
            render::RenderSkybox(framebuffer, activeCamera, *sceneView.ibl->skybox);
            break;
        case render::SkyboxDisplayMode::Irradiance:
            render::RenderSkybox(framebuffer, activeCamera, *sceneView.ibl->irradiance);
            break;
        case render::SkyboxDisplayMode::Prefilter:
            render::RenderSkybox(
                framebuffer,
                activeCamera,
                *sceneView.ibl->prefilter,
                static_cast<float>(m_PrefilterPreviewLod));
            break;
        }
    }

    framebuffer.ClearDepth();
    for (std::size_t index = 0; index < sceneView.partCount; ++index) {
        const RenderScenePart& part = sceneView.parts[index];
        if (part.mesh == nullptr) {
            continue;
        }

        switch (m_RenderMode) {
        case RenderMode::Blinn:
            RenderBlinnMeshPart(
                framebuffer,
                part,
                activeCamera.ProjectionMat4(),
                activeCamera.ViewMat4(),
                *sceneView.model,
                activeCamera.Pos,
                lightPos);
            break;
        case RenderMode::DirectPbr:
            RenderDirectPbrMeshPart(
                framebuffer,
                part,
                activeCamera.ProjectionMat4(),
                activeCamera.ViewMat4(),
                *sceneView.model,
                activeCamera.Pos,
                lightPos);
            break;
        case RenderMode::IblPbr:
            if (sceneView.ibl != nullptr && sceneView.ibl->Ready()) {
                RenderIblPbrMeshPart(
                    framebuffer,
                    part,
                    *sceneView.ibl,
                    activeCamera.ProjectionMat4(),
                    activeCamera.ViewMat4(),
                    *sceneView.model,
                    activeCamera.Pos,
                    lightPos);
            } else {
                RenderDirectPbrMeshPart(
                    framebuffer,
                    part,
                    activeCamera.ProjectionMat4(),
                    activeCamera.ViewMat4(),
                    *sceneView.model,
                    activeCamera.Pos,
                    lightPos);
            }
            break;
        }
    }

    framebuffer.ResolveColor();
}

} // namespace sr
