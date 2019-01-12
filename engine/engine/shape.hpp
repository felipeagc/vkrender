#pragma once

#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>

namespace engine {
class Shape {
public:
  std::vector<uint32_t> m_indices;
  std::vector<re_vertex_t> m_vertices;
};

class BoxShape : public Shape {
public:
  BoxShape(const glm::vec3 &a, const glm::vec3 &b) {
    m_vertices.resize(8);

    m_vertices[0].pos = a;
    m_vertices[1].pos = glm::vec3(a.x, b.y, a.z);
    m_vertices[2].pos = glm::vec3(a.x, b.y, b.z);
    m_vertices[3].pos = glm::vec3(a.x, a.y, b.z);
    m_vertices[4].pos = glm::vec3(b.x, a.y, a.z);
    m_vertices[5].pos = glm::vec3(b.x, b.y, a.z);
    m_vertices[6].pos = b;
    m_vertices[7].pos = glm::vec3(b.x, a.y, b.z);

    m_indices = {
        0, 2, 1, //
        0, 3, 2, //
        3, 6, 2, //
        3, 7, 6, //
        7, 5, 6, //
        7, 4, 5, //
        4, 1, 5, //
        4, 0, 1, //
        1, 6, 5, //
        1, 2, 6, //
        0, 7, 4, //
        0, 3, 7, //
    };
  }
};
} // namespace engine
