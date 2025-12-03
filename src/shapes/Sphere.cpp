#include "sphere.h"
#include <glm/gtc/constants.hpp>

void Sphere::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = std::max(2, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight) {

    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, glm::normalize(topLeft));
    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, glm::normalize(bottomLeft));
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, glm::normalize(bottomRight));

    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, glm::normalize(topLeft));
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, glm::normalize(bottomRight));
    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, glm::normalize(topRight));
}

static inline glm::vec3 sphericalPoint(float phi, float theta, float r=0.5f) {
    return glm::vec3 { r * glm::sin(phi) * glm::cos(theta),
                     r * glm::cos(phi),
                     -r * glm::sin(phi) * glm::sin(theta) };
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {

    const int stacks = m_param1;
    const float dphi = glm::pi<float>() / stacks;

    for (int i=0; i < stacks; ++i) {
        float phi0 = i * dphi;
        float phi1 = (i+1) * dphi;

        glm::vec3 tl = sphericalPoint(phi0, currentTheta);
        glm::vec3 tr = sphericalPoint(phi0, nextTheta);
        glm::vec3 bl = sphericalPoint(phi1, currentTheta);
        glm::vec3 br = sphericalPoint(phi1, nextTheta);

        makeTile(tl, tr, bl, br);
    }
}

void Sphere::makeSphere() {

    const float thetaStep = glm::radians(360.f / m_param2);

    for (int k=0; k < m_param2; ++k) {
        float theta0 = k * thetaStep;
        float theta1 = (k+1) * thetaStep;
        makeWedge(theta0, theta1);
    }

}

void Sphere::setVertexData() {
    m_vertexData.clear();
    makeSphere();
}

void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
