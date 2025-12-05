#include "Cube.h"

void Cube::updateParams(int param1) {
    m_param1 = std::max(1, param1);
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight,
                    glm::vec2 uvTopLeft,
                    glm::vec2 uvTopRight,
                    glm::vec2 uvBottomLeft,
                    glm::vec2 uvBottomRight) {
    
    glm::vec3 N = glm::normalize(glm::cross(bottomLeft - topLeft, bottomRight - topLeft));

    // triangle 1
    Vertex v1;
    v1.position = topLeft;
    v1.normal = N;
    v1.uv = uvTopLeft;
    m_vertices.push_back(v1);
    
    Vertex v2;
    v2.position = bottomLeft;
    v2.normal = N;
    v2.uv = uvBottomLeft;
    m_vertices.push_back(v2);
    
    Vertex v3;
    v3.position = bottomRight;
    v3.normal = N;
    v3.uv = uvBottomRight;
    m_vertices.push_back(v3);

    // triangle 2
    Vertex v4;
    v4.position = topLeft;
    v4.normal = N;
    v4.uv = uvTopLeft;
    m_vertices.push_back(v4);
    
    Vertex v5;
    v5.position = bottomRight;
    v5.normal = N;
    v5.uv = uvBottomRight;
    m_vertices.push_back(v5);
    
    Vertex v6;
    v6.position = topRight;
    v6.normal = N;
    v6.uv = uvTopRight;
    m_vertices.push_back(v6);
}

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {

    const float step = 1.0f / m_param1;

    for (int r = 0; r < m_param1; ++r) {
        float v0 = r * step;
        float v1 = (r+1) * step;

        glm::vec3 L0 = glm::mix(topLeft, bottomLeft, v0);
        glm::vec3 R0 = glm::mix(topRight, bottomRight, v0);
        glm::vec3 L1 = glm::mix(topLeft, bottomLeft, v1);
        glm::vec3 R1 = glm::mix(topRight, bottomRight, v1);

        for (int c = 0; c < m_param1; ++c) {
            float u0 = c * step;
            float u1 = (c+1) * step;

            // corners
            glm::vec3 tl = glm::mix(L0, R0, u0);
            glm::vec3 tr = glm::mix(L0, R0, u1);
            glm::vec3 bl = glm::mix(L1, R1, u0);
            glm::vec3 br = glm::mix(L1, R1, u1);

            // UV coordinates
            glm::vec2 uvTL(u0, v0);
            glm::vec2 uvTR(u1, v0);
            glm::vec2 uvBL(u0, v1);
            glm::vec2 uvBR(u1, v1);

            makeTile(tl, tr, bl, br, uvTL, uvTR, uvBL, uvBR);
        }
    }
}

void Cube::buildVertices() {
    // +Z face (front)
    makeFace(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, 0.5f),
             glm::vec3( 0.5f, -0.5f, 0.5f));

    // -Z face (back)
    makeFace(glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f));

    // +X (right)
    makeFace(glm::vec3( 0.5f,  0.5f,  0.5f),
             glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f,  0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f));

    // -X (left)
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f,  0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f,  0.5f));

    // +Y (top)
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f,  0.5f),
             glm::vec3( 0.5f,  0.5f,  0.5f));

    // -Y (bottom)
    makeFace(glm::vec3(-0.5f, -0.5f,  0.5f),
             glm::vec3( 0.5f, -0.5f,  0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f));
}
