#include "app/Application.h"

#include "base/Math.h"

#include "platform/Platform.h"
#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/Pipeline.h"
#include "render/Texture2D.h"
#include "render/Vertex.h"
#include "resource/loaders/ObjMeshLoader.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>
#include <utility>
#include <vector>

#ifndef SR_PROJECT_ROOT
#define SR_PROJECT_ROOT "."
#endif

namespace sr {
namespace {

#pragma pack(push, 1)
struct BmpFileHeader {
    std::uint16_t type = 0x4D42;
    std::uint32_t size = 0;
    std::uint16_t reserved1 = 0;
    std::uint16_t reserved2 = 0;
    std::uint32_t offset = 0;
};

struct BmpInfoHeader {
    std::uint32_t size = 40;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint16_t planes = 1;
    std::uint16_t bitCount = 24;
    std::uint32_t compression = 0;
    std::uint32_t sizeImage = 0;
    std::int32_t xPelsPerMeter = 2835;
    std::int32_t yPelsPerMeter = 2835;
    std::uint32_t clrUsed = 0;
    std::uint32_t clrImportant = 0;
};
#pragma pack(pop)

constexpr std::size_t kCubeFaceCount = 6;
constexpr float kFpsReportIntervalSeconds = 0.5f;
constexpr const char* kPreviousScreenshotName = "previous-face-debug-flat.bmp";
constexpr const char* kCurrentScreenshotName = "current-face-debug-lit.bmp";

enum class CubeFaceId {
    Front = 0,
    Right = 1,
    Back = 2,
    Left = 3,
    Top = 4,
    Bottom = 5,
    Count = 6,
};

struct TriangleVertices {
    std::array<render::LitVertex, 3> vertices{};
};

struct ProjectedBounds {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
};

struct FaceDebugDraw {
    render::Mesh<render::DebugFaceVertex> mesh;
    render::Color color{};
    int number = 0;
};

std::filesystem::path GetDefaultModelPath()
{
    return std::filesystem::path{ SR_PROJECT_ROOT } / "assets" / "models" / "cube.obj";
}

std::filesystem::path GetScreenshotsDirectory()
{
    return std::filesystem::path{ SR_PROJECT_ROOT } / "docs" / "screenshots";
}

constexpr std::size_t FaceIndex(CubeFaceId face)
{
    return static_cast<std::size_t>(face);
}

bool ShouldExportDemoImagesOnly()
{
    const char* env = std::getenv("SR_EXPORT_DEMO_IMAGES");
    return env != nullptr && env[0] != '\0' && env[0] != '0';
}


CubeFaceId DetermineCubeFace(const math::Vec3& p0,
    const math::Vec3& p1,
    const math::Vec3& p2)
{
    constexpr float kPlaneEpsilon = 1e-4f;

    const float minX = std::min({ p0.x, p1.x, p2.x });
    const float maxX = std::max({ p0.x, p1.x, p2.x });
    const float minY = std::min({ p0.y, p1.y, p2.y });
    const float maxY = std::max({ p0.y, p1.y, p2.y });
    const float minZ = std::min({ p0.z, p1.z, p2.z });
    const float maxZ = std::max({ p0.z, p1.z, p2.z });

    if (std::abs(maxX - minX) <= kPlaneEpsilon) {
        return ((p0.x + p1.x + p2.x) / 3.0f) >= 0.0f ? CubeFaceId::Right : CubeFaceId::Left;
    }

    if (std::abs(maxY - minY) <= kPlaneEpsilon) {
        return ((p0.y + p1.y + p2.y) / 3.0f) >= 0.0f ? CubeFaceId::Top : CubeFaceId::Bottom;
    }

    if (std::abs(maxZ - minZ) <= kPlaneEpsilon) {
        return ((p0.z + p1.z + p2.z) / 3.0f) >= 0.0f ? CubeFaceId::Front : CubeFaceId::Back;
    }

    return CubeFaceId::Count;
}

math::Vec2 ProjectToFacePlane(const math::Vec3& position, CubeFaceId face)
{
    switch (face) {
    case CubeFaceId::Front:
        return { position.x, position.y };
    case CubeFaceId::Right:
        return { -position.z, position.y };
    case CubeFaceId::Back:
        return { -position.x, position.y };
    case CubeFaceId::Left:
        return { position.z, position.y };
    case CubeFaceId::Top:
        return { position.x, -position.z };
    case CubeFaceId::Bottom:
        return { position.x, position.z };
    case CubeFaceId::Count:
        break;
    }

    return {};
}

ProjectedBounds CalculateProjectedBounds(const std::vector<TriangleVertices>& triangles,
    CubeFaceId face)
{
    ProjectedBounds bounds;
    for (const TriangleVertices& triangle : triangles) {
        for (const render::LitVertex& vertex : triangle.vertices) {
            const math::Vec2 projected = ProjectToFacePlane(vertex.positionModel, face);
            bounds.minX = std::min(bounds.minX, projected.x);
            bounds.maxX = std::max(bounds.maxX, projected.x);
            bounds.minY = std::min(bounds.minY, projected.y);
            bounds.maxY = std::max(bounds.maxY, projected.y);
        }
    }

    return bounds;
}

float NormalizeCoord(float value, float minValue, float maxValue)
{
    const float range = maxValue - minValue;
    if (std::abs(range) <= 1e-6f) {
        return 0.5f;
    }

    return math::Clamp((value - minValue) / range, 0.0f, 1.0f);
}

math::Vec2 BuildFaceUv(const math::Vec3& position,
    CubeFaceId face,
    const ProjectedBounds& bounds)
{
    const math::Vec2 projected = ProjectToFacePlane(position, face);
    return {
        NormalizeCoord(projected.x, bounds.minX, bounds.maxX),
        NormalizeCoord(projected.y, bounds.minY, bounds.maxY),
    };
}

FaceDebugDraw BuildFaceDebugDraw(const std::vector<TriangleVertices>& triangles,
    CubeFaceId face,
    const render::Color& color,
    int number)
{
    const ProjectedBounds bounds = CalculateProjectedBounds(triangles, face);

    std::vector<render::DebugFaceVertex> vertices;
    std::vector<render::Mesh<render::DebugFaceVertex>::index_t> indices;
    vertices.reserve(triangles.size() * 3);
    indices.reserve(triangles.size() * 3);

    render::Mesh<render::DebugFaceVertex>::index_t nextIndex = 0;
    for (const TriangleVertices& triangle : triangles) {
        for (const render::LitVertex& sourceVertex : triangle.vertices) {
            vertices.emplace_back(
                sourceVertex.positionModel,
                sourceVertex.normalModel,
                BuildFaceUv(sourceVertex.positionModel, face, bounds));
            indices.push_back(nextIndex++);
        }
    }

    return FaceDebugDraw{
        render::Mesh<render::DebugFaceVertex>(std::move(vertices), std::move(indices)),
        color,
        number,
    };
}

bool BuildFaceDebugDraws(const render::Mesh<render::LitVertex>& sourceMesh,
    std::vector<FaceDebugDraw>& draws,
    std::string& error)
{
    std::array<std::vector<TriangleVertices>, kCubeFaceCount> faceTriangles;

    sourceMesh.ProcessMesh([&](const render::LitVertex& v0,
                               const render::LitVertex& v1,
                               const render::LitVertex& v2) {
        const CubeFaceId face = DetermineCubeFace(v0.positionModel, v1.positionModel, v2.positionModel);
        if (face == CubeFaceId::Count) {
            return;
        }

        faceTriangles[FaceIndex(face)].push_back(TriangleVertices{
            std::array<render::LitVertex, 3>{ v0, v1, v2 }
        });
    });

    constexpr std::array<render::Color, kCubeFaceCount> kFaceColors{
        render::Color{ 231, 111, 81 },
        render::Color{ 244, 162, 97 },
        render::Color{ 233, 196, 106 },
        render::Color{ 42, 157, 143 },
        render::Color{ 38, 70, 83 },
        render::Color{ 122, 92, 151 },
    };

    draws.clear();
    draws.reserve(kCubeFaceCount);

    for (std::size_t index = 0; index < kCubeFaceCount; ++index) {
        if (faceTriangles[index].empty()) {
            error = "The loaded OBJ is missing one or more cube faces needed for debug numbering.";
            return false;
        }

        draws.push_back(BuildFaceDebugDraw(
            faceTriangles[index],
            static_cast<CubeFaceId>(index),
            kFaceColors[index],
            static_cast<int>(index) + 1));
    }

    return true;
}

void RenderFaceDebugScene(render::Framebuffer& framebuffer,
    const std::vector<FaceDebugDraw>& faceDraws,
    const render::Color& clearColor,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos,
    bool lightingEnabled)
{
    framebuffer.Clear(clearColor);
    framebuffer.ClearDepth();

    render::Program<render::DebugFaceVertex,
        render::DebugFaceUniforms,
        render::DebugFaceVaryings> program{};
    program.vertexShader = &render::DebugFaceVertexShader;
    program.fragmentShader = &render::DebugFaceFragmentShader;
    program.faceCullMode = render::FaceCullMode::None;

    for (const FaceDebugDraw& faceDraw : faceDraws) {
        render::DebugFaceUniforms uniforms{};
        uniforms.mvp = projection * view * model;
        uniforms.model = model;
        uniforms.color = faceDraw.color;
        uniforms.faceNumber = faceDraw.number;
        uniforms.lightPosWorld = lightPos;
        uniforms.cameraPosWorld = cameraPos;
        uniforms.lightingEnabled = lightingEnabled ? 1.0f : 0.0f;

        render::Pipeline<render::DebugFaceVertex,
            render::DebugFaceUniforms,
            render::DebugFaceVaryings> pipeline(program, uniforms);
        pipeline.Run(framebuffer, faceDraw.mesh);
    }
}

void RenderTexturedScene(render::Framebuffer& framebuffer,
    const render::Mesh<render::LitVertex>& mesh,
    const render::Texture2D& texture,
    const render::Color& clearColor,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos)
{
    framebuffer.Clear(clearColor);
    framebuffer.ClearDepth();

    render::Program<render::LitVertex,
        render::LitUniforms,
        render::LitVaryings> program{};
    program.vertexShader = &render::LitVertexShader;
    program.fragmentShader = &render::LitFragmentShader;
    program.faceCullMode = render::FaceCullMode::Back;

    render::LitUniforms uniforms{};
    uniforms.mvp = projection * view * model;
    uniforms.model = model;
    uniforms.color = render::Color{ 255, 255, 255 };
    uniforms.lightPosWorld = lightPos;
    uniforms.cameraPosWorld = cameraPos;
    uniforms.baseColorTexture = &texture;
    uniforms.textureEnabled = 1.0f;

    render::Pipeline<render::LitVertex,
        render::LitUniforms,
        render::LitVaryings> pipeline(program, uniforms);
    pipeline.Run(framebuffer, mesh);
}

bool WriteFramebufferToBmp(const std::filesystem::path& path,
    const render::Framebuffer& framebuffer,
    std::string& error)
{
    const int width = framebuffer.Width();
    const int height = framebuffer.Height();
    if (width <= 0 || height <= 0) {
        error = "Invalid framebuffer size for screenshot export.";
        return false;
    }

    const std::uint32_t rowStride = static_cast<std::uint32_t>(((width * 3) + 3) & ~3);
    const std::uint32_t imageSize = rowStride * static_cast<std::uint32_t>(height);

    BmpFileHeader fileHeader;
    fileHeader.offset = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
    fileHeader.size = fileHeader.offset + imageSize;

    BmpInfoHeader infoHeader;
    infoHeader.width = width;
    infoHeader.height = height;
    infoHeader.sizeImage = imageSize;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error = "Failed to open screenshot path for writing: " + path.string();
        return false;
    }

    out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    std::vector<std::uint8_t> row(static_cast<std::size_t>(rowStride), 0U);
    for (int y = height - 1; y >= 0; --y) {
        std::fill(row.begin(), row.end(), 0U);
        for (int x = 0; x < width; ++x) {
            const std::uint32_t pixel = framebuffer.GetPixel(x, y);
            const std::size_t offset = static_cast<std::size_t>(x * 3);
            row[offset + 0] = static_cast<std::uint8_t>(pixel & 0xFFU);
            row[offset + 1] = static_cast<std::uint8_t>((pixel >> 8U) & 0xFFU);
            row[offset + 2] = static_cast<std::uint8_t>((pixel >> 16U) & 0xFFU);
        }

        out.write(reinterpret_cast<const char*>(row.data()), static_cast<std::streamsize>(row.size()));
    }

    if (!out.good()) {
        error = "Failed while writing screenshot file: " + path.string();
        return false;
    }

    return true;
}

bool ExportDemoImages(const std::vector<FaceDebugDraw>& faceDraws,
    int width,
    int height,
    const math::Mat4& projection,
    const math::Mat4& view,
    const math::Mat4& model,
    const math::Vec3& cameraPos,
    const math::Vec3& lightPos,
    std::string& error)
{
    render::Framebuffer previousFramebuffer(width, height);
    render::Framebuffer currentFramebuffer(width, height);

    RenderFaceDebugScene(previousFramebuffer, faceDraws, CameraLayer::Get().BackgroundColor(), projection, view, model, cameraPos, lightPos, false);
    RenderFaceDebugScene(currentFramebuffer, faceDraws, CameraLayer::Get().BackgroundColor(), projection, view, model, cameraPos, lightPos, true);

    const std::filesystem::path screenshotsDir = GetScreenshotsDirectory();
    const std::filesystem::path previousPath = screenshotsDir / kPreviousScreenshotName;
    const std::filesystem::path currentPath = screenshotsDir / kCurrentScreenshotName;

    if (!WriteFramebufferToBmp(previousPath, previousFramebuffer, error)) {
        return false;
    }

    if (!WriteFramebufferToBmp(currentPath, currentFramebuffer, error)) {
        return false;
    }

    std::cout << "Saved previous demo image to: " << previousPath.string() << '\n';
    std::cout << "Saved current demo image to: " << currentPath.string() << '\n';
    return true;
}

} // namespace

Application::Application(std::string title, int width, int height)
    : m_Title(std::move(title)), m_Width(width), m_Height(height)
{
}

int Application::Run()
{
    const std::filesystem::path modelPath = GetDefaultModelPath();
    const resource::ObjLitLoadResult loadResult = resource::LoadLitMesh(modelPath);
    if (!loadResult.Ok()) {
        std::cerr << loadResult.error << '\n';
        return 1;
    }

    render::Framebuffer framebuffer(m_Width, m_Height);
    m_CameraLayer.OnAttach(framebuffer);

    const render::Texture2D& baseColorTexture = loadResult.baseColorTexture;
    const math::Vec3 lightPos{ 1.6f, 2.0f, 2.8f };
    const math::Mat4 model = math::RotateY(math::kPi / 5.0f) * math::RotateX(-math::kPi / 7.0f);
    const render::Camera& camera = m_CameraLayer.Camera();

    if (ShouldExportDemoImagesOnly()) {
        std::vector<FaceDebugDraw> faceDraws;
        std::string faceBuildError;
        if (!BuildFaceDebugDraws(loadResult.mesh, faceDraws, faceBuildError)) {
            std::cerr << faceBuildError << '\n';
            return 1;
        }

        std::string exportError;
        if (!ExportDemoImages(faceDraws,
                m_Width,
                m_Height,
                camera.ProjectionMat4(),
                camera.ViewMat4(),
                model,
                camera.Pos,
                lightPos,
                exportError)) {
            std::cerr << exportError << '\n';
            return 1;
        }
        return 0;
    }

    if (!platform::Platform::Initialize(m_Title, m_Width, m_Height)) {
        std::cerr << "Failed to create main window on platform: "
                  << platform::Platform::Name() << '\n';
        platform::Platform::Close();
        return 1;
    }

    std::cout << "SoftRenderer OBJ textured viewer started on "
              << platform::Platform::Name()
              << " with model \"" << modelPath.string() << "\" ("
              << loadResult.mesh.Vertices().size() << " expanded vertices, "
              << loadResult.mesh.Indices().size() / 3 << " triangles, material \""
              << loadResult.materialName << "\", texture \""
              << loadResult.baseColorTexturePath.string() << "\" ("
              << baseColorTexture.Width() << "x" << baseColorTexture.Height() << ")\n"
              << "Controls: WASD move, Space/Left Shift move vertically, Q/E turn\n";

    platform::Platform::Show();

    auto previousFrameTime = std::chrono::steady_clock::now();
    int fpsFrameCount = 0;
    float fpsAccumulatedSeconds = 0.0f;
    float renderAccumulatedMs = 0.0f;
    float presentAccumulatedMs = 0.0f;
    bool wroteFpsLine = false;

    while (platform::Platform::ProcessEvents()) {
        const auto currentFrameTime = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentFrameTime - previousFrameTime).count();
        previousFrameTime = currentFrameTime;

        m_CameraLayer.OnUpdate(deltaTime);

        const render::Camera& activeCamera = m_CameraLayer.Camera();
        const auto renderStartTime = std::chrono::steady_clock::now();
        RenderTexturedScene(
            framebuffer,
            loadResult.mesh,
            baseColorTexture,
            m_CameraLayer.BackgroundColor(),
            activeCamera.ProjectionMat4(),
            activeCamera.ViewMat4(),
            model,
            activeCamera.Pos,
            lightPos);
        const auto renderEndTime = std::chrono::steady_clock::now();
        renderAccumulatedMs += std::chrono::duration<float, std::milli>(renderEndTime - renderStartTime).count();

        const auto presentStartTime = renderEndTime;
        if (!platform::Platform::Present(framebuffer)) {
            std::cerr << "Failed to present framebuffer to window.\n";
            platform::Platform::Close();
            return 1;
        }
        const auto presentEndTime = std::chrono::steady_clock::now();
        presentAccumulatedMs += std::chrono::duration<float, std::milli>(presentEndTime - presentStartTime).count();

        fpsAccumulatedSeconds += deltaTime;
        ++fpsFrameCount;
        if (fpsAccumulatedSeconds >= kFpsReportIntervalSeconds) {
            const float averageFps = fpsFrameCount > 0 && fpsAccumulatedSeconds > 0.0f
                ? static_cast<float>(fpsFrameCount) / fpsAccumulatedSeconds
                : 0.0f;
            const float averageFrameMs = fpsFrameCount > 0
                ? (fpsAccumulatedSeconds * 1000.0f) / static_cast<float>(fpsFrameCount)
                : 0.0f;
            const float averageRenderMs = fpsFrameCount > 0
                ? renderAccumulatedMs / static_cast<float>(fpsFrameCount)
                : 0.0f;
            const float averagePresentMs = fpsFrameCount > 0
                ? presentAccumulatedMs / static_cast<float>(fpsFrameCount)
                : 0.0f;

            std::cout << "\rFPS: "
                      << std::fixed << std::setprecision(1)
                      << averageFps
                      << " | Frame: "
                      << averageFrameMs
                      << " ms | Render: "
                      << averageRenderMs
                      << " ms | Present: "
                      << averagePresentMs
                      << " ms"
                      << std::flush;

            fpsAccumulatedSeconds = 0.0f;
            fpsFrameCount = 0;
            renderAccumulatedMs = 0.0f;
            presentAccumulatedMs = 0.0f;
            wroteFpsLine = true;
        }
    }

    if (wroteFpsLine) {
        std::cout << '\n';
    }

    platform::Platform::Close();
    return 0;
}

} // namespace sr

