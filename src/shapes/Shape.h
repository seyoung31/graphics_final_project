#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    Vertex()
        : position(0.0f), normal(0.0f), uv(0.0f), tangent(0.0f), bitangent(0.0f) {}
};

class Shape {
public:
    void insertVec3(std::vector<float> &data, glm::vec3 v);
    void setVertexData() {
        m_vertices = std::vector<Vertex>();
        m_vertexData = std::vector<float>();
        buildVertices();
        computeTangents();
        flattenVertices();
    }

protected:
    virtual void buildVertices() = 0;
    void computeTangents();
    void flattenVertices();
    
    std::vector<Vertex> m_vertices;
    std::vector<float> m_vertexData;
};

