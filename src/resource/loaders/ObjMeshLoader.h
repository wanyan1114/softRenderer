#pragma once

#include "render/Mesh.h"
#include "render/Texture2D.h"
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
    render::Texture2D baseColorTexture;
    std::filesystem::path baseColorTexturePath;
    std::string materialName;
    std::string error;

    bool Ok() const { return error.empty(); }
};

ObjLoadResult LoadFlatColorMesh(const std::filesystem::path& path);
ObjLitLoadResult LoadLitMesh(const std::filesystem::path& path);

} // namespace sr::resource
