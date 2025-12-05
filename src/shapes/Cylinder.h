#pragma once

#include "Shape.h"
#include <vector>
#include <glm/glm.hpp>

class Cylinder : public Shape
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void buildVertices() override;
    void makeTopCapSlice(int radialIdx);
    void makeBottomCapSlice(int radialIdx);
    void makeSideSlice(int radialIdx);

    int m_param1;
    int m_param2;
    float m_radius = 0.5f;
};
