#pragma once

#include "render/Vertex.h"

#include <initializer_list>
#include <vector>

namespace sr::render {

struct Triangle {
    Vertex vertices[3];

    Triangle() = default;
    Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
        : vertices{ v0, v1, v2 }
    {
    }
};

class Mesh {
public:
    Mesh() = default;
    Mesh(std::initializer_list<Triangle> triangles)
        : m_Triangles(triangles)
    {
    }

    void AddTriangle(const Triangle& triangle)
    {
        m_Triangles.push_back(triangle);
    }

    bool Empty() const { return m_Triangles.empty(); }
    const std::vector<Triangle>& Triangles() const { return m_Triangles; }

private:
    std::vector<Triangle> m_Triangles;
};

} // namespace sr::render
