#pragma once

#include <vector>
#include <glm/glm.hpp>

class Cylinder
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void insertVec3(std::vector<float> &data, glm::vec3 v);
    void setVertexData();

    void makeCapSlice(float currentTheta, float nextTheta, bool isCap);
    void makeCapTile(glm::vec3 outerA,
                               glm::vec3 outerB,
                               glm::vec3 innerA,
                               glm::vec3 innerB,
                     const glm::vec3& desiredNormal);
    glm::vec3 calcNorm(glm::vec3& pt);
    void makeSlopeSlice(float currentTheta, float nextTheta);
    void makeWedge(float currentTheta, float nextTheta);


    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5;
};
