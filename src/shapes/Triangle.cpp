#include "Triangle.h"

void Triangle::updateParams() {
    m_vertexData = std::vector<float>();
    setVertexData();
}

void Triangle::setVertexData() {
    // Task 1: update m_vertexData with the vertices and normals
    //         needed to tesselate a triangle

    /**
     * (-0.5, 0.5, 0.0)
(-0.5, -0.5, 0.0)
(0.5, -0.5, 0.0)
     */
    insertVec3(m_vertexData, glm::vec3(-0.5, 0.5, 0.0));//coord
    insertVec3(m_vertexData, glm::vec3(0.0, 0.0, 0.5));
    insertVec3(m_vertexData, glm::vec3(-0.5, -0.5, 0.0));//coord

    insertVec3(m_vertexData, glm::vec3(0.0, 0.0, 0.5));
    insertVec3(m_vertexData, glm::vec3(0.5, -0.5, 0.0));//coord

    insertVec3(m_vertexData, glm::vec3(0.0, 0.0, 0.5));
    // Note: you may find the insertVec3 function useful in adding your points into m_vertexData
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Triangle::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
