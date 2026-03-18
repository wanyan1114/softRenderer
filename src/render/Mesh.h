#pragma once

#include "render/Vertex.h"

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
    const std::vector<vertex_t>& Vertices() const { return m_Vertices; }
    const std::vector<index_t>& Indices() const { return m_Indices; }

private:
    std::vector<vertex_t> m_Vertices;
    std::vector<index_t> m_Indices;
};

} // namespace sr::render
