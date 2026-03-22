#pragma once

#include "render/Color.h"
#include "render/Mesh.h"
#include "render/Texture2D.h"
#include "render/VertexTypes.h"

#include <filesystem>
#include <string>

namespace sr::resource {

struct ObjLitLoadResult {
    render::Mesh<render::LitVertex> mesh;
    render::Texture2D baseColorTexture;
    std::filesystem::path baseColorTexturePath;
    std::string materialName;
    std::string error;

    bool Ok() const { return error.empty(); }
};

ObjLitLoadResult LoadLitMesh(const std::filesystem::path& path);

} // namespace sr::resource
