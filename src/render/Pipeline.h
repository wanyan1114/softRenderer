#pragma once

#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/PipelineStages.h"
#include "render/Vertex.h"

#include <cstddef>
#include <type_traits>
#include <utility>

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
        static_assert(std::is_trivially_copyable_v<varyings_t>,
            "varyings_t must be trivially copyable");
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

        ProcessMesh(mesh, framebuffer);
    }

private:
    using VertexStage = detail::VertexStage<vertex_t, uniforms_t, varyings_t>;
    using ClipStage = detail::ClipStage<varyings_t>;
    using RasterStage = detail::RasterStage<varyings_t>;
    using FragmentStage = detail::FragmentStage<vertex_t, uniforms_t, varyings_t>;

    bool CanRun(const Mesh<vertex_t>& mesh) const
    {
        return !mesh.Empty()
            && mesh.HasCompleteTriangles()
            && VertexStage::ProgramAvailable(m_Program);
    }

    void ProcessMesh(const Mesh<vertex_t>& mesh,
        Framebuffer& framebuffer) const
    {
        mesh.ProcessMesh([&](const vertex_t& v0, const vertex_t& v1, const vertex_t& v2) {
            ProcessInputTriangle(framebuffer, v0, v1, v2);
        });
    }

    void ProcessInputTriangle(Framebuffer& framebuffer,
        const vertex_t& vertex0,
        const vertex_t& vertex1,
        const vertex_t& vertex2) const
    {
        const auto inputTriangle = VertexStage::Execute(
            m_Program,
            m_Uniforms,
            vertex0,
            vertex1,
            vertex2);

        const auto clippedTriangles = ClipStage::Execute(inputTriangle);
        for (const auto& clippedTriangle : clippedTriangles) {
            RasterStage::Execute(
                framebuffer,
                clippedTriangle,
                m_Program.faceCullMode,
                [&](std::size_t pixelIndex, const varyings_t& varyings) {
                    FragmentStage::Execute(
                        pixelIndex,
                        varyings,
                        m_Program,
                        m_Uniforms,
                        framebuffer);
                });
        }
    }

    Program<vertex_t, uniforms_t, varyings_t> m_Program;
    uniforms_t m_Uniforms;
};

} // namespace sr::render
