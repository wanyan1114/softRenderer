#include "resource/loaders/ObjMeshLoader.h"

#include "base/Math.h"

#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace sr::resource {
namespace {

struct ParsedFaceVertex {
    render::Mesh<render::FlatColorVertex>::index_t positionIndex = 0;
    render::Mesh<render::FlatColorVertex>::index_t normalIndex = 0;
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

bool ParseIndexToken(const std::string& token,
    render::Mesh<render::FlatColorVertex>::index_t& index)
{
    if (token.empty()) {
        return false;
    }

    std::istringstream stream(token);
    int parsedIndex = 0;
    if (!(stream >> parsedIndex) || parsedIndex <= 0) {
        return false;
    }

    index = static_cast<render::Mesh<render::FlatColorVertex>::index_t>(parsedIndex - 1);
    return true;
}

bool ParseVertexLine(const std::string& line, sr::math::Vec3& position)
{
    std::istringstream stream(line.substr(2));
    return static_cast<bool>(stream >> position.x >> position.y >> position.z);
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
        out.hasNormal = false;
        return ParseIndexToken(token, out.positionIndex);
    }

    const std::string positionToken = token.substr(0, firstSlash);
    if (!ParseIndexToken(positionToken, out.positionIndex)) {
        return false;
    }

    const std::size_t lastSlash = token.rfind('/');
    if (lastSlash == std::string::npos || lastSlash + 1 >= token.size()) {
        out.hasNormal = false;
        return true;
    }

    const std::string normalToken = token.substr(lastSlash + 1);
    if (!ParseIndexToken(normalToken, out.normalIndex)) {
        return false;
    }

    out.hasNormal = true;
    return true;
}

bool ParseFaceLine(const std::string& line,
    std::array<ParsedFaceVertex, 3>& faceVertices)
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

bool PositionIndexInRange(render::Mesh<render::FlatColorVertex>::index_t index,
    const std::vector<math::Vec3>& positions)
{
    return static_cast<std::size_t>(index) < positions.size();
}

bool NormalIndexInRange(render::Mesh<render::FlatColorVertex>::index_t index,
    const std::vector<math::Vec3>& normals)
{
    return static_cast<std::size_t>(index) < normals.size();
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
    std::vector<render::Mesh<render::FlatColorVertex>::index_t> indices;
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
        return { {}, "Failed to open OBJ file: " + path.string() };
    }

    std::vector<math::Vec3> positions;
    std::vector<math::Vec3> normals;
    std::vector<render::LitVertex> vertices;
    std::vector<render::Mesh<render::LitVertex>::index_t> indices;
    std::string line;
    int lineNumber = 0;
    render::Mesh<render::LitVertex>::index_t nextIndex = 0;

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
            continue;
        }

        if (trimmed.rfind("vn ", 0) == 0) {
            math::Vec3 normal{};
            if (!ParseNormalLine(trimmed, normal)) {
                return { {}, "Invalid normal line at " + std::to_string(lineNumber) + " in " + path.string() };
            }

            normals.push_back(normal.Normalized());
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
                if (!faceVertex.hasNormal) {
                    return { {}, "OBJ face is missing a normal index in: " + path.string() };
                }
                if (!NormalIndexInRange(faceVertex.normalIndex, normals)) {
                    return { {}, "OBJ face references an out-of-range normal index in: " + path.string() };
                }

                vertices.emplace_back(
                    positions[static_cast<std::size_t>(faceVertex.positionIndex)],
                    normals[static_cast<std::size_t>(faceVertex.normalIndex)]);
                indices.push_back(nextIndex++);
            }
        }
    }

    if (positions.empty()) {
        return { {}, "OBJ file contains no vertices: " + path.string() };
    }

    if (normals.empty()) {
        return { {}, "OBJ file contains no normals: " + path.string() };
    }

    if (indices.empty()) {
        return { {}, "OBJ file contains no faces: " + path.string() };
    }

    return { render::Mesh<render::LitVertex>(std::move(vertices), std::move(indices)), {} };
}

} // namespace sr::resource
