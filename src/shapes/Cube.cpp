#include "Cube.h"
#include "Triangle.h"

void Cube::updateParams(int param1) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 2: create a tile (i.e. 2 triangles) based on 4 given points.

    glm::vec3 u = topRight - topLeft;
    glm::vec3 v = bottomLeft - topLeft;
    glm::vec3 normal = -glm::normalize(glm::cross(u, v));

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 3: create a single side of the cube out of the 4
    //         given points and makeTile()
    // Note: think about how param 1 affects the number of triangles on
    //       the face of the cube
    glm::vec3 horizStep = (topRight - topLeft) / (float)m_param1;
    glm::vec3 vertStep = (bottomLeft - topLeft) / (float)m_param1;

    // Subdivide face into m_param1 x m_param1 tiles
    for (int i = 0; i < m_param1; i++) {
        for (int j = 0; j < m_param1; j++) {
            // Compute the 4 corners of this small tile
            glm::vec3 tl = topLeft + (float)i * vertStep + (float)j * horizStep;
            glm::vec3 tr = topLeft + (float)i * vertStep + (float)(j + 1) * horizStep;
            glm::vec3 bl = topLeft + (float)(i + 1) * vertStep + (float)j * horizStep;
            glm::vec3 br = topLeft + (float)(i + 1) * vertStep + (float)(j + 1) * horizStep;

            // Make the tile
            makeTile(tl, tr, bl, br);
        }
    }


}

void Cube::setVertexData() {
    // Uncomment these lines for Task 2, then comment them out for Task 3:

    // makeTile(glm::vec3(-0.5f,  0.5f, 0.5f),
    //          glm::vec3( 0.5f,  0.5f, 0.5f),
    //          glm::vec3(-0.5f, -0.5f, 0.5f),
    //          glm::vec3( 0.5f, -0.5f, 0.5f));

    // Uncomment these lines for Task 3:

    // FRONT (+Z)
    makeFace(glm::vec3(-0.5f,  0.5f,  0.5f),  // top-left
             glm::vec3( 0.5f,  0.5f,  0.5f),  // top-right
             glm::vec3(-0.5f, -0.5f,  0.5f),  // bottom-left
             glm::vec3( 0.5f, -0.5f,  0.5f)); // bottom-right

    // BACK (−Z)
    makeFace(glm::vec3( 0.5f,  0.5f, -0.5f),  // top-left
             glm::vec3(-0.5f,  0.5f, -0.5f),  // top-right
             glm::vec3( 0.5f, -0.5f, -0.5f),  // bottom-left
             glm::vec3(-0.5f, -0.5f, -0.5f)); // bottom-right

    // LEFT (−X)
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),  // top-left
             glm::vec3(-0.5f,  0.5f,  0.5f),  // top-right
             glm::vec3(-0.5f, -0.5f, -0.5f),  // bottom-left
             glm::vec3(-0.5f, -0.5f,  0.5f)); // bottom-right

    // RIGHT (+X)
    makeFace(glm::vec3( 0.5f,  0.5f,  0.5f),  // top-left
             glm::vec3( 0.5f,  0.5f, -0.5f),  // top-right
             glm::vec3( 0.5f, -0.5f,  0.5f),  // bottom-left
             glm::vec3( 0.5f, -0.5f, -0.5f)); // bottom-right

    // TOP (+Y)
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),  // top-left
             glm::vec3( 0.5f,  0.5f, -0.5f),  // top-right
             glm::vec3(-0.5f,  0.5f,  0.5f),  // bottom-left
             glm::vec3( 0.5f,  0.5f,  0.5f)); // bottom-right

    // BOTTOM (−Y)
    makeFace(glm::vec3(-0.5f, -0.5f,  0.5f),  // top-left
             glm::vec3( 0.5f, -0.5f,  0.5f),  // top-right
             glm::vec3(-0.5f, -0.5f, -0.5f),  // bottom-left
             glm::vec3( 0.5f, -0.5f, -0.5f)); // bottom-right



    // Task 4: Use the makeFace() function to make all 6 sides of the cube

}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
