#include "cube.h"

void Cube::updateParams(int param1) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {

    glm::vec3 N = glm::normalize(glm::cross(bottomLeft - topLeft, bottomRight - topLeft));

    // triangle 1
    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, N);
    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, N);
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, N);

    // triangle 2
    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, N);
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, N);
    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, N);

}

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {

    m_vertexData.reserve(m_vertexData.size() + 6 * 6 * m_param1 * m_param1);
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

            makeTile(tl, tr, bl, br);
        }
    }


}

void Cube::setVertexData() {
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

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
