#pragma once

#include "Shape.h"
#include <vector>
#include <glm/glm.hpp>

class Cube : public Shape
{
public:
    void updateParams(int param1);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void buildVertices() override;
    void makeTile(glm::vec3 topLeft,
                  glm::vec3 topRight,
                  glm::vec3 bottomLeft,
                  glm::vec3 bottomRight,
                  glm::vec2 uvTopLeft,
                  glm::vec2 uvTopRight,
                  glm::vec2 uvBottomLeft,
                  glm::vec2 uvBottomRight);
    void makeFace(glm::vec3 topLeft,
                  glm::vec3 topRight,
                  glm::vec3 bottomLeft,
                  glm::vec3 bottomRight);

    int m_param1;
};
