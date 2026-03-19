#pragma once

#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/PipelineStages.h"
#include "render/Vertex.h"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace sr::render {

template<typename vertex_t, typename uniforms_t, typename varyings_t>
class Pipeline {
public:
    Pipeline(Program<vertex_t, uniforms_t, varyings_t> program, uniforms_t uniforms)
        : m_Program(std::move(program)),
          m_Uniforms(std::move(uniforms))
    {
        static_assert(std::is_base_of_v<VertexBase, vertex_t>,
            "vertex_t must derive from VertexBase");
        static_assert(std::is_base_of_v<UniformsBase, uniforms_t>,
            "uniforms_t must derive from UniformsBase");
        static_assert(std::is_base_of_v<VaryingsBase, varyings_t>,
            "varyings_t must derive from VaryingsBase");
        static_assert(std::is_standard_layout_v<varyings_t>,
            "varyings_t must be standard layout");
        static_assert(sizeof(varyings_t) % sizeof(float) == 0,
            "varyings_t must contain only float-compatible fields");
    }

    uniforms_t& Uniforms() { return m_Uniforms; }
    const uniforms_t& Uniforms() const { return m_Uniforms; }

    void Run(Framebuffer& framebuffer, const Mesh<vertex_t>& mesh) const
    {
        if (!CanRun(mesh)) {
            return;
        }

        StagedFrame stagedFrame = InitializeFrame(framebuffer);
        ProcessMesh(mesh, framebuffer, stagedFrame);
        CommitFrame(framebuffer, stagedFrame);
    }

private:
    using VertexStage = detail::VertexStage<vertex_t, uniforms_t, varyings_t>;
    using ClipStage = detail::ClipStage<varyings_t>;
    using RasterStage = detail::RasterStage<varyings_t>;
    using FragmentStage = detail::FragmentStage<vertex_t, uniforms_t, varyings_t>;
    using StagedFrame = detail::StagedFrame;

    bool CanRun(const Mesh<vertex_t>& mesh) const
    {
        return !mesh.Empty()
            && mesh.HasCompleteTriangles()
            && VertexStage::ProgramAvailable(m_Program);
    }

    static std::size_t PixelIndex(int width, int x, int y)
    {
        return static_cast<std::size_t>(y * width + x);
    }

    StagedFrame InitializeFrame(const Framebuffer& framebuffer) const
    {
        const int width = framebuffer.Width();
        const int height = framebuffer.Height();
        StagedFrame stagedFrame{
            std::vector<float>(static_cast<std::size_t>(width * height), 1.0f),
            std::vector<Color>(static_cast<std::size_t>(width * height)),
            std::vector<bool>(static_cast<std::size_t>(width * height), false),
        };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                stagedFrame.depth[PixelIndex(width, x, y)] = framebuffer.GetDepth(x, y);
            }
        }

        return stagedFrame;
    }

    void ProcessMesh(const Mesh<vertex_t>& mesh,
        const Framebuffer& framebuffer,
        StagedFrame& stagedFrame) const
    {
        mesh.ProcessMesh([&](const vertex_t& v0, const vertex_t& v1, const vertex_t& v2) {
            ProcessInputTriangle(framebuffer, v0, v1, v2, stagedFrame);
        });
    }

    void ProcessInputTriangle(const Framebuffer& framebuffer,
        const vertex_t& vertex0,
        const vertex_t& vertex1,
        const vertex_t& vertex2,
        StagedFrame& stagedFrame) const
    {
        const auto inputTriangle = VertexStage::Execute(
            m_Program,
            m_Uniforms,
            vertex0,
            vertex1,
            vertex2);

        const auto clippedTriangles = ClipStage::Execute(inputTriangle);
        for (const auto& clippedTriangle : clippedTriangles) {
            const auto fragments = RasterStage::Execute(
                framebuffer,
                clippedTriangle,
                m_Program.faceCullMode);
            FragmentStage::Execute(
                fragments,
                m_Program,
                m_Uniforms,
                framebuffer.Width(),
                stagedFrame);
        }
    }

    static void CommitFrame(Framebuffer& framebuffer, const StagedFrame& stagedFrame)
    {
        const int width = framebuffer.Width();
        const int height = framebuffer.Height();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t index = PixelIndex(width, x, y);
                if (!stagedFrame.coverage[index]) {
                    continue;
                }

                framebuffer.SetDepth(x, y, stagedFrame.depth[index]);
                framebuffer.SetPixel(x, y, stagedFrame.color[index]);
            }
        }
    }

    Program<vertex_t, uniforms_t, varyings_t> m_Program;
    uniforms_t m_Uniforms;
};

} // namespace sr::render
