#pragma once

#include "render/Mesh.h"
#include "render/Vertex.h"

#include <filesystem>
#include <string>

namespace sr::resource {

struct ObjLoadResult {
    render::Mesh<render::FlatColorVertex> mesh;
    std::string error;

    bool Ok() const { return error.empty(); }
};

struct ObjLitLoadResult {
    render::Mesh<render::LitVertex> mesh;
    std::string error;

    bool Ok() const { return error.empty(); }
};

ObjLoadResult LoadFlatColorMesh(const std::filesystem::path& path);
ObjLitLoadResult LoadLitMesh(const std::filesystem::path& path);

} // namespace sr::resource
