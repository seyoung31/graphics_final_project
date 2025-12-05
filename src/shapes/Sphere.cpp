#include "Sphere.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

void Sphere::updateParams(int param1, int param2) {
    m_param1 = std::max(2, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight,
                      float currentTheta, float nextTheta,
                      float phi1, float phi2) {
    glm::vec3 normalTopLeft = glm::normalize(topLeft);
    glm::vec3 normalTopRight = glm::normalize(topRight);
    glm::vec3 normalBottomLeft = glm::normalize(bottomLeft);
    glm::vec3 normalBottomRight = glm::normalize(bottomRight);
    
    float u1 = currentTheta / (2.0f * glm::pi<float>());
    float u2 = nextTheta / (2.0f * glm::pi<float>());
    
    float v1 = phi1 / glm::pi<float>();
    float v2 = phi2 / glm::pi<float>();
    
    // Handle poles (v = 0 or v = 1)
    float u1_pole = u1;
    float u2_pole = u2;
    if (v1 < 0.001f || v1 > 0.999f) {
        u1_pole = (u1 + u2) * 0.5f;
    }
    if (v2 < 0.001f || v2 > 0.999f) {
        u2_pole = (u1 + u2) * 0.5f;
    }
    
    glm::vec2 uvTopLeft = glm::vec2(u1, v1);
    glm::vec2 uvTopRight = glm::vec2(u2, v1);
    glm::vec2 uvBottomLeft = glm::vec2(u1, v2);
    glm::vec2 uvBottomRight = glm::vec2(u2, v2);
    
    // First triangle
    Vertex v1_vert;
    v1_vert.position = topLeft;
    v1_vert.normal = normalTopLeft;
    v1_vert.uv = uvTopLeft;
    m_vertices.push_back(v1_vert);
    
    Vertex v2_vert;
    v2_vert.position = bottomLeft;
    v2_vert.normal = normalBottomLeft;
    v2_vert.uv = uvBottomLeft;
    m_vertices.push_back(v2_vert);
    
    Vertex v3_vert;
    v3_vert.position = bottomRight;
    v3_vert.normal = normalBottomRight;
    v3_vert.uv = uvBottomRight;
    m_vertices.push_back(v3_vert);
    
    // Second triangle
    Vertex v4_vert;
    v4_vert.position = topLeft;
    v4_vert.normal = normalTopLeft;
    v4_vert.uv = uvTopLeft;
    m_vertices.push_back(v4_vert);
    
    Vertex v5_vert;
    v5_vert.position = bottomRight;
    v5_vert.normal = normalBottomRight;
    v5_vert.uv = uvBottomRight;
    m_vertices.push_back(v5_vert);
    
    Vertex v6_vert;
    v6_vert.position = topRight;
    v6_vert.normal = normalTopRight;
    v6_vert.uv = uvTopRight;
    m_vertices.push_back(v6_vert);
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {
    float phiStep = glm::pi<float>() / m_param1;
    
    for (int i = 0; i < m_param1; i++) {
        float phi1 = i * phiStep;
        float phi2 = (i + 1) * phiStep;
        
        glm::vec3 topLeft(
            m_radius * glm::sin(phi1) * glm::cos(currentTheta),
            m_radius * glm::cos(phi1),
            -m_radius * glm::sin(phi1) * glm::sin(currentTheta)
        );
        
        glm::vec3 topRight(
            m_radius * glm::sin(phi1) * glm::cos(nextTheta),
            m_radius * glm::cos(phi1),
            -m_radius * glm::sin(phi1) * glm::sin(nextTheta)
        );
        
        glm::vec3 bottomLeft(
            m_radius * glm::sin(phi2) * glm::cos(currentTheta),
            m_radius * glm::cos(phi2),
            -m_radius * glm::sin(phi2) * glm::sin(currentTheta)
        );
        
        glm::vec3 bottomRight(
            m_radius * glm::sin(phi2) * glm::cos(nextTheta),
            m_radius * glm::cos(phi2),
            -m_radius * glm::sin(phi2) * glm::sin(nextTheta)
        );
        
        makeTile(topLeft, topRight, bottomLeft, bottomRight, currentTheta, nextTheta, phi1, phi2);
    }
}

void Sphere::makeSphere() {
    float thetaStep = glm::radians(360.f / m_param2);
    
    for (int i = 0; i < m_param2; i++) {
        float currentTheta = i * thetaStep;
        float nextTheta = (i + 1) * thetaStep;
        makeWedge(currentTheta, nextTheta);
    }
}

void Sphere::buildVertices() {
    makeSphere();
}
