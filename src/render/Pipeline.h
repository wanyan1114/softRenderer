#pragma once

#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/PipelineStages.h"
#include "render/Program.h"
#include "render/VertexTypes.h"
#include "render/shader/ShaderTypes.h"

#include <array>
#include <type_traits>

namespace sr::render {

template<typename vertex_t, typename uniforms_t, typename varyings_t>
class Pipeline {
public:
    void Run(Framebuffer& framebuffer,
        const Mesh<vertex_t>& mesh,
        const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms) const
    {
        if (!MeshReady(mesh) || !detail::ProgramAvailable(program)) {
            return;
        }

        mesh.ProcessMesh([&](const vertex_t& v0, const vertex_t& v1, const vertex_t& v2) {
            RunTriangle(framebuffer, v0, v1, v2, program, uniforms);
        });
    }

    void RunTriangle(Framebuffer& framebuffer,
        const vertex_t& vertex0,
        const vertex_t& vertex1,
        const vertex_t& vertex2,
        const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms) const
    {
        if (!detail::ProgramAvailable(program)) {
            return;
        }

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

        detail::PipelineStageMachine<vertex_t, uniforms_t, varyings_t> stageMachine(
            framebuffer,
            program,
            uniforms,
            std::array<vertex_t, 3>{ vertex0, vertex1, vertex2 });
        stageMachine.Run();
    }

private:
    static bool MeshReady(const Mesh<vertex_t>& mesh)
    {
        return !mesh.Empty() && mesh.HasCompleteTriangles();
    }
};

} // namespace sr::render
