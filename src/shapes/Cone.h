#pragma once

#include <vector>
#include <glm/glm.hpp>

class Cone
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:

    void insertVec3(std::vector<float> &data, glm::vec3 v);
    void setVertexData();
    void makeCapTile(glm::vec3 topLeft,
                     glm::vec3 topRight,
                     glm::vec3 bottomLeft,
                     glm::vec3 bottomRight);
    void makeCapSlice(float currentTheta, float nextTheta);
    void makeSlopeTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight,
                       glm::vec3 nTL, glm::vec3 nTR,
                       glm::vec3 nBL, glm::vec3 nBR);
    void makeSlopeSlice(float currentTheta, float nextTheta);
    void makeWedge(float currentTheta, float nextTheta);

    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5;
};
