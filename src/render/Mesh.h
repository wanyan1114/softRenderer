#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace sr::render {

template<typename vertex_t>
class Mesh {
public:
    using index_t = std::uint32_t;

    Mesh() = default;

    Mesh(std::vector<vertex_t> vertices, std::vector<index_t> indices)
        : m_Vertices(std::move(vertices)),
          m_Indices(std::move(indices))
    {
    }

    Mesh(std::initializer_list<vertex_t> vertices, std::initializer_list<index_t> indices)
        : m_Vertices(vertices),
          m_Indices(indices)
    {
    }

    void AddVertex(const vertex_t& vertex)
    {
        m_Vertices.push_back(vertex);
    }

    void AddTriangle(index_t i0, index_t i1, index_t i2)
    {
        m_Indices.push_back(i0);
        m_Indices.push_back(i1);
        m_Indices.push_back(i2);
    }

    bool Empty() const { return m_Vertices.empty() || m_Indices.empty(); }
    bool HasCompleteTriangles() const { return m_Indices.size() >= 3 && m_Indices.size() % 3 == 0; }
    const std::vector<vertex_t>& Vertices() const { return m_Vertices; }
    const std::vector<index_t>& Indices() const { return m_Indices; }

    template<typename visitor_t>
    void ProcessMesh(visitor_t&& visitor) const
    {
        if (!HasCompleteTriangles()) {
            return;
        }

        for (std::size_t indexOffset = 0; indexOffset < m_Indices.size(); indexOffset += 3) {
            const vertex_t* v0 = nullptr;
            const vertex_t* v1 = nullptr;
            const vertex_t* v2 = nullptr;
            if (!TryGetTriangleVertices(indexOffset, v0, v1, v2)) {
                continue;
            }

            visitor(*v0, *v1, *v2);
        }
    }

private:
    bool TryGetTriangleVertices(std::size_t indexOffset,
        const vertex_t*& v0,
        const vertex_t*& v1,
        const vertex_t*& v2) const
    {
        if (indexOffset + 2 >= m_Indices.size()) {
            return false;
        }

        const index_t i0 = m_Indices[indexOffset + 0];
        const index_t i1 = m_Indices[indexOffset + 1];
        const index_t i2 = m_Indices[indexOffset + 2];
        if (!AreIndicesValid(i0, i1, i2)) {
            return false;
        }

        v0 = &m_Vertices[static_cast<std::size_t>(i0)];
        v1 = &m_Vertices[static_cast<std::size_t>(i1)];
        v2 = &m_Vertices[static_cast<std::size_t>(i2)];
        return true;
    }

    bool AreIndicesValid(index_t i0, index_t i1, index_t i2) const
    {
        const std::size_t vertexCount = m_Vertices.size();
        return static_cast<std::size_t>(i0) < vertexCount
            && static_cast<std::size_t>(i1) < vertexCount
            && static_cast<std::size_t>(i2) < vertexCount;
    }

    std::vector<vertex_t> m_Vertices;
    std::vector<index_t> m_Indices;
};

} // namespace sr::render
