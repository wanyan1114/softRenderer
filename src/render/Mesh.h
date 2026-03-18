#pragma once

#include "render/Vertex.h"

#include <initializer_list>
#include <vector>

namespace sr::render {

template<typename vertex_t>
struct Triangle {
    vertex_t vertices[3];

    Triangle() = default;
    Triangle(const vertex_t& v0, const vertex_t& v1, const vertex_t& v2)
        : vertices{ v0, v1, v2 }
    {
    }
};

template<typename vertex_t>
class Mesh {
public:
    Mesh() = default;
    Mesh(std::initializer_list<Triangle<vertex_t>> triangles)
        : m_Triangles(triangles)
    {
    }

    void AddTriangle(const Triangle<vertex_t>& triangle)
    {
        m_Triangles.push_back(triangle);
    }

    bool Empty() const { return m_Triangles.empty(); }
    const std::vector<Triangle<vertex_t>>& Triangles() const { return m_Triangles; }

private:
    std::vector<Triangle<vertex_t>> m_Triangles;
};

} // namespace sr::render
