#include "resource/loaders/ObjMeshLoader.h"

#include "base/Math.h"
#include "resource/loaders/ImageTextureLoader.h"

#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace sr::resource {
namespace {

using FlatMeshIndex = render::Mesh<render::FlatColorVertex>::index_t;
using LitMeshIndex = render::Mesh<render::LitVertex>::index_t;

struct ParsedFaceVertex {
    FlatMeshIndex positionIndex = 0;
    FlatMeshIndex texCoordIndex = 0;
    FlatMeshIndex normalIndex = 0;
    bool hasTexCoord = false;
    bool hasNormal = false;
};

bool IsWhitespaceOnly(const std::string& text)
{
    for (char ch : text) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

std::string TrimLeadingWhitespace(const std::string& text)
{
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    return text.substr(start);
}

std::string Trim(const std::string& text)
{
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

bool ParseIndexToken(const std::string& token, FlatMeshIndex& index)
{
    if (token.empty()) {
        return false;
    }

    std::istringstream stream(token);
    int parsedIndex = 0;
    if (!(stream >> parsedIndex) || parsedIndex <= 0) {
        return false;
    }

    index = static_cast<FlatMeshIndex>(parsedIndex - 1);
    return true;
}

bool ParseVertexLine(const std::string& line, sr::math::Vec3& position)
{
    std::istringstream stream(line.substr(2));
    return static_cast<bool>(stream >> position.x >> position.y >> position.z);
}

bool ParseTexCoordLine(const std::string& line, sr::math::Vec2& texCoord)
{
    std::istringstream stream(line.substr(3));
    return static_cast<bool>(stream >> texCoord.x >> texCoord.y);
}

bool ParseNormalLine(const std::string& line, sr::math::Vec3& normal)
{
    std::istringstream stream(line.substr(3));
    return static_cast<bool>(stream >> normal.x >> normal.y >> normal.z);
}

bool ParseFaceVertexToken(const std::string& token, ParsedFaceVertex& out)
{
    const std::size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos) {
        out.hasTexCoord = false;
        out.hasNormal = false;
        return ParseIndexToken(token, out.positionIndex);
    }

    if (!ParseIndexToken(token.substr(0, firstSlash), out.positionIndex)) {
        return false;
    }

    const std::size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos) {
        const std::string texCoordToken = token.substr(firstSlash + 1);
        if (!texCoordToken.empty()) {
            if (!ParseIndexToken(texCoordToken, out.texCoordIndex)) {
                return false;
            }
            out.hasTexCoord = true;
        }
        out.hasNormal = false;
        return true;
    }

    const std::string texCoordToken = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
    if (!texCoordToken.empty()) {
        if (!ParseIndexToken(texCoordToken, out.texCoordIndex)) {
            return false;
        }
        out.hasTexCoord = true;
    }

    const std::string normalToken = token.substr(secondSlash + 1);
    if (!normalToken.empty()) {
        if (!ParseIndexToken(normalToken, out.normalIndex)) {
            return false;
        }
        out.hasNormal = true;
    }

    return true;
}

bool ParseFaceLine(const std::string& line, std::array<ParsedFaceVertex, 3>& faceVertices)
{
    std::istringstream stream(line.substr(2));
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() != 3) {
        return false;
    }

    return ParseFaceVertexToken(tokens[0], faceVertices[0])
        && ParseFaceVertexToken(tokens[1], faceVertices[1])
        && ParseFaceVertexToken(tokens[2], faceVertices[2]);
}

bool PositionIndexInRange(FlatMeshIndex index, const std::vector<math::Vec3>& positions)
{
    return static_cast<std::size_t>(index) < positions.size();
}

bool TexCoordIndexInRange(FlatMeshIndex index, const std::vector<math::Vec2>& texCoords)
{
    return static_cast<std::size_t>(index) < texCoords.size();
}

bool NormalIndexInRange(FlatMeshIndex index, const std::vector<math::Vec3>& normals)
{
    return static_cast<std::size_t>(index) < normals.size();
}

bool ParseObjPathReference(const std::string& line, const char* prefix, std::filesystem::path& outPath)
{
    const std::string value = Trim(line.substr(std::char_traits<char>::length(prefix)));
    if (value.empty()) {
        return false;
    }

    outPath = std::filesystem::path(value);
    return true;
}

bool ParseMaterialNameReference(const std::string& line, const char* prefix, std::string& outName)
{
    outName = Trim(line.substr(std::char_traits<char>::length(prefix)));
    return !outName.empty();
}

struct MaterialLibraryParseResult {
    std::filesystem::path baseColorTexturePath;
    std::string error;

    bool Ok() const { return error.empty(); }
};

MaterialLibraryParseResult ResolveBaseColorTexturePath(
    const std::filesystem::path& materialLibraryPath,
    const std::string& materialName)
{
    std::ifstream file(materialLibraryPath);
    if (!file.is_open()) {
        return { {}, "Failed to open MTL file: " + materialLibraryPath.string() };
    }

    std::unordered_map<std::string, std::filesystem::path> materialTexturePaths;
    std::string currentMaterialName;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        const std::string trimmed = TrimLeadingWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#' || IsWhitespaceOnly(trimmed)) {
            continue;
        }

        if (trimmed.rfind("newmtl ", 0) == 0) {
            if (!ParseMaterialNameReference(trimmed, "newmtl ", currentMaterialName)) {
                return { {}, "Invalid newmtl line at " + std::to_string(lineNumber) + " in " + materialLibraryPath.string() };
            }
            continue;
        }

        if (trimmed.rfind("map_Kd ", 0) == 0) {
            if (currentMaterialName.empty()) {
                return { {}, "map_Kd appears before newmtl at line " + std::to_string(lineNumber) + " in " + materialLibraryPath.string() };
            }

            std::filesystem::path texturePath;
            if (!ParseObjPathReference(trimmed, "map_Kd ", texturePath)) {
                return { {}, "Invalid map_Kd line at " + std::to_string(lineNumber) + " in " + materialLibraryPath.string() };
            }

            const std::string texturePathString = texturePath.string();
            if (!texturePathString.empty() && texturePathString[0] == '-') {
                return { {}, "map_Kd options are not supported in minimal loader: " + materialLibraryPath.string() };
            }

            materialTexturePaths[currentMaterialName] = texturePath;
        }
    }

    const auto it = materialTexturePaths.find(materialName);
    if (it == materialTexturePaths.end()) {
        return { {}, "Material '" + materialName + "' does not define map_Kd in " + materialLibraryPath.string() };
    }

    return { materialLibraryPath.parent_path() / it->second, {} };
}

} // namespace

ObjLoadResult LoadFlatColorMesh(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return { {}, "Failed to open OBJ file: " + path.string() };
    }

    std::vector<math::Vec3> positions;
    std::vector<render::FlatColorVertex> vertices;
    std::vector<FlatMeshIndex> indices;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        const std::string trimmed = TrimLeadingWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#' || IsWhitespaceOnly(trimmed)) {
            continue;
        }

        if (trimmed.rfind("v ", 0) == 0) {
            math::Vec3 position{};
            if (!ParseVertexLine(trimmed, position)) {
                return { {}, "Invalid vertex line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            positions.push_back(position);
            vertices.emplace_back(position);
            continue;
        }

        if (trimmed.rfind("f ", 0) == 0) {
            std::array<ParsedFaceVertex, 3> faceVertices{};
            if (!ParseFaceLine(trimmed, faceVertices)) {
                return { {}, "Only triangular faces are supported. Invalid face line at "
                        + std::to_string(lineNumber) + " in " + path.string() };
            }

            for (const ParsedFaceVertex& faceVertex : faceVertices) {
                if (!PositionIndexInRange(faceVertex.positionIndex, positions)) {
                    return { {}, "OBJ face references an out-of-range vertex index in: " + path.string() };
                }
                indices.push_back(faceVertex.positionIndex);
            }
        }
    }

    if (vertices.empty()) {
        return { {}, "OBJ file contains no vertices: " + path.string() };
    }

    if (indices.empty()) {
        return { {}, "OBJ file contains no faces: " + path.string() };
    }

    return { render::Mesh<render::FlatColorVertex>(std::move(vertices), std::move(indices)), {} };
}

ObjLitLoadResult LoadLitMesh(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return { {}, {}, {}, {}, "Failed to open OBJ file: " + path.string() };
    }

    std::vector<math::Vec3> positions;
    std::vector<math::Vec2> texCoords;
    std::vector<math::Vec3> normals;
    std::vector<render::LitVertex> vertices;
    std::vector<LitMeshIndex> indices;
    std::string line;
    int lineNumber = 0;
    LitMeshIndex nextIndex = 0;
    std::filesystem::path materialLibraryPath;
    std::string activeMaterialName;
    std::string resolvedMaterialName;

    while (std::getline(file, line)) {
        ++lineNumber;
        const std::string trimmed = TrimLeadingWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#' || IsWhitespaceOnly(trimmed)) {
            continue;
        }

        if (trimmed.rfind("mtllib ", 0) == 0) {
            std::filesystem::path relativeMaterialPath;
            if (!ParseObjPathReference(trimmed, "mtllib ", relativeMaterialPath)) {
                return { {}, {}, {}, {}, "Invalid mtllib line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            if (!materialLibraryPath.empty() && materialLibraryPath != path.parent_path() / relativeMaterialPath) {
                return { {}, {}, {}, {}, "Only a single mtllib is supported in minimal loader: " + path.string() };
            }

            materialLibraryPath = path.parent_path() / relativeMaterialPath;
            continue;
        }

        if (trimmed.rfind("usemtl ", 0) == 0) {
            if (!ParseMaterialNameReference(trimmed, "usemtl ", activeMaterialName)) {
                return { {}, {}, {}, {}, "Invalid usemtl line at " + std::to_string(lineNumber) + " in " + path.string() };
            }
            continue;
        }

        if (trimmed.rfind("v ", 0) == 0) {
            math::Vec3 position{};
            if (!ParseVertexLine(trimmed, position)) {
                return { {}, {}, {}, {}, "Invalid vertex line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            positions.push_back(position);
            continue;
        }

        if (trimmed.rfind("vt ", 0) == 0) {
            math::Vec2 texCoord{};
            if (!ParseTexCoordLine(trimmed, texCoord)) {
                return { {}, {}, {}, {}, "Invalid texcoord line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            texCoords.push_back(texCoord);
            continue;
        }

        if (trimmed.rfind("vn ", 0) == 0) {
            math::Vec3 normal{};
            if (!ParseNormalLine(trimmed, normal)) {
                return { {}, {}, {}, {}, "Invalid normal line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            normals.push_back(normal.Normalized());
            continue;
        }

        if (trimmed.rfind("f ", 0) == 0) {
            if (activeMaterialName.empty()) {
                return { {}, {}, {}, {}, "Faces must appear after usemtl in minimal loader: " + path.string() };
            }

            if (resolvedMaterialName.empty()) {
                resolvedMaterialName = activeMaterialName;
            } else if (resolvedMaterialName != activeMaterialName) {
                return { {}, {}, {}, {}, "Only a single material is supported for one OBJ in minimal loader: " + path.string() };
            }

            std::array<ParsedFaceVertex, 3> faceVertices{};
            if (!ParseFaceLine(trimmed, faceVertices)) {
                return { {}, {}, {}, {}, "Only triangular faces are supported. Invalid face line at "
                        + std::to_string(lineNumber) + " in " + path.string() };
            }

            for (const ParsedFaceVertex& faceVertex : faceVertices) {
                if (!PositionIndexInRange(faceVertex.positionIndex, positions)) {
                    return { {}, {}, {}, {}, "OBJ face references an out-of-range vertex index in: " + path.string() };
                }
                if (!faceVertex.hasTexCoord) {
                    return { {}, {}, {}, {}, "OBJ face is missing a texcoord index in: " + path.string() };
                }
                if (!TexCoordIndexInRange(faceVertex.texCoordIndex, texCoords)) {
                    return { {}, {}, {}, {}, "OBJ face references an out-of-range texcoord index in: " + path.string() };
                }
                if (!faceVertex.hasNormal) {
                    return { {}, {}, {}, {}, "OBJ face is missing a normal index in: " + path.string() };
                }
                if (!NormalIndexInRange(faceVertex.normalIndex, normals)) {
                    return { {}, {}, {}, {}, "OBJ face references an out-of-range normal index in: " + path.string() };
                }

                vertices.emplace_back(
                    positions[static_cast<std::size_t>(faceVertex.positionIndex)],
                    normals[static_cast<std::size_t>(faceVertex.normalIndex)],
                    texCoords[static_cast<std::size_t>(faceVertex.texCoordIndex)]);
                indices.push_back(nextIndex++);
            }
        }
    }

    if (positions.empty()) {
        return { {}, {}, {}, {}, "OBJ file contains no vertices: " + path.string() };
    }

    if (texCoords.empty()) {
        return { {}, {}, {}, {}, "OBJ file contains no texcoords: " + path.string() };
    }

    if (normals.empty()) {
        return { {}, {}, {}, {}, "OBJ file contains no normals: " + path.string() };
    }

    if (indices.empty()) {
        return { {}, {}, {}, {}, "OBJ file contains no faces: " + path.string() };
    }

    if (materialLibraryPath.empty()) {
        return { {}, {}, {}, {}, "OBJ file is missing mtllib: " + path.string() };
    }

    if (resolvedMaterialName.empty()) {
        return { {}, {}, {}, {}, "OBJ file does not use any material: " + path.string() };
    }

    const MaterialLibraryParseResult materialResult = ResolveBaseColorTexturePath(materialLibraryPath, resolvedMaterialName);
    if (!materialResult.Ok()) {
        return { {}, {}, {}, {}, materialResult.error };
    }

    const ImageTextureLoadResult textureResult = LoadImageTexture2D(materialResult.baseColorTexturePath);
    if (!textureResult.Ok()) {
        return { {}, {}, {}, {}, textureResult.error };
    }

    return {
        render::Mesh<render::LitVertex>(std::move(vertices), std::move(indices)),
        textureResult.texture,
        materialResult.baseColorTexturePath,
        resolvedMaterialName,
        {}
    };
}

} // namespace sr::resource


