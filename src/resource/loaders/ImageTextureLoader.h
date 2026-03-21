#pragma once

#include "render/Texture2D.h"

#include <filesystem>
#include <string>

namespace sr::resource {

struct ImageTextureLoadResult {
    render::Texture2D texture;
    std::string error;

    bool Ok() const { return error.empty(); }
};

bool IsSupportedBaseColorTexturePath(const std::filesystem::path& path);
ImageTextureLoadResult LoadImageTexture2D(const std::filesystem::path& path);

} // namespace sr::resource
