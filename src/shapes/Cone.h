#pragma once

#include "Shape.h"
#include <vector>
#include <glm/glm.hpp>

class Cone : public Shape
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void buildVertices() override;
    void makeCapTile(glm::vec3 topLeft,
                     glm::vec3 topRight,
                     glm::vec3 bottomLeft,
                     glm::vec3 bottomRight,
                     float theta0, float theta1,
                     float r0, float r1);
    void makeCapSlice(float currentTheta, float nextTheta);
    void makeSlopeTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight,
                       glm::vec3 nTL, glm::vec3 nTR,
                       glm::vec3 nBL, glm::vec3 nBR,
                       float theta0, float theta1,
                       float v0, float v1);
    void makeSlopeSlice(float currentTheta, float nextTheta);
    void makeWedge(float currentTheta, float nextTheta);

    int m_param1;
    int m_param2;
    float m_radius = 0.5f;
};
