#pragma once

#include "Shape.h"
#include <vector>
#include <glm/glm.hpp>

class Sphere : public Shape
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void buildVertices() override;
    void makeTile(glm::vec3 topLeft,
                  glm::vec3 topRight,
                  glm::vec3 bottomLeft,
                  glm::vec3 bottomRight,
                  float currentTheta, float nextTheta,
                  float phi1, float phi2);
    void makeWedge(float currTheta, float nextTheta);
    void makeSphere();

    float m_radius = 0.5f;
    int m_param1;
    int m_param2;
};
