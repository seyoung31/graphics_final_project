#include "Cone.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

void Cone::updateParams(int param1, int param2) {
    m_param1 = std::max(1, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

static inline glm::vec3 calcNorm(const glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);
    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

static inline float coneRadiusAtY(float y) {
    return 0.5f * (0.5f - y);
}

void Cone::makeCapTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight,
                       float theta0, float theta1,
                       float r0, float r1) {
    glm::vec3 n(0.f, -1.f, 0.f);
    
    // UV coordinates for cap (circular mapping)
    float u0 = theta0 / (2.0f * glm::pi<float>());
    float u1 = theta1 / (2.0f * glm::pi<float>());
    float v0_uv = r0 / m_radius;
    float v1_uv = r1 / m_radius;
    
    // tri1: tl -> bl -> tr
    Vertex v1; v1.position = topLeft; v1.normal = n; v1.uv = glm::vec2(u0, v0_uv);
    Vertex v2; v2.position = bottomLeft; v2.normal = n; v2.uv = glm::vec2(u0, v1_uv);
    Vertex v3; v3.position = topRight; v3.normal = n; v3.uv = glm::vec2(u1, v0_uv);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
    m_vertices.push_back(v3);

    // tri2: tr -> bl -> br
    Vertex v4; v4.position = topRight; v4.normal = n; v4.uv = glm::vec2(u1, v0_uv);
    Vertex v5; v5.position = bottomLeft; v5.normal = n; v5.uv = glm::vec2(u0, v1_uv);
    Vertex v6; v6.position = bottomRight; v6.normal = n; v6.uv = glm::vec2(u1, v1_uv);
    m_vertices.push_back(v4);
    m_vertices.push_back(v5);
    m_vertices.push_back(v6);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta) {
    const float step = 0.5f / m_param1;

    for (int i = 0; i < m_param1; ++i) {
        float r0 = i * step;
        float r1 = (i+1) * step;

        glm::vec3 tl(r0 * glm::cos(currentTheta), -0.5f, r0 * glm::sin(currentTheta));
        glm::vec3 tr(r0 * glm::cos(nextTheta), -0.5f, r0 * glm::sin(nextTheta));
        glm::vec3 bl(r1 * glm::cos(currentTheta), -0.5f, r1 * glm::sin(currentTheta));
        glm::vec3 br(r1 * glm::cos(nextTheta), -0.5f, r1 * glm::sin(nextTheta));

        makeCapTile(tl, tr, bl, br, currentTheta, nextTheta, r0, r1);
    }
}

void Cone::makeSlopeTile(glm::vec3 topLeft,
                         glm::vec3 topRight,
                         glm::vec3 bottomLeft,
                         glm::vec3 bottomRight,
                         glm::vec3 nTL, glm::vec3 nTR,
                         glm::vec3 nBL, glm::vec3 nBR,
                         float theta0, float theta1,
                         float v0, float v1) {
    // UV mapping for slope
    float u0 = theta0 / (2.0f * glm::pi<float>());
    float u1 = theta1 / (2.0f * glm::pi<float>());
    
    // tri1: TL -> TR -> BL
    Vertex vert1; vert1.position = topLeft; vert1.normal = nTL; vert1.uv = glm::vec2(u0, v0);
    Vertex vert2; vert2.position = topRight; vert2.normal = nTR; vert2.uv = glm::vec2(u1, v0);
    Vertex vert3; vert3.position = bottomLeft; vert3.normal = nBL; vert3.uv = glm::vec2(u0, v1);
    m_vertices.push_back(vert1);
    m_vertices.push_back(vert2);
    m_vertices.push_back(vert3);

    // tri2: BL -> TR -> BR
    Vertex vert4; vert4.position = bottomLeft; vert4.normal = nBL; vert4.uv = glm::vec2(u0, v1);
    Vertex vert5; vert5.position = topRight; vert5.normal = nTR; vert5.uv = glm::vec2(u1, v0);
    Vertex vert6; vert6.position = bottomRight; vert6.normal = nBR; vert6.uv = glm::vec2(u1, v1);
    m_vertices.push_back(vert4);
    m_vertices.push_back(vert5);
    m_vertices.push_back(vert6);
}

void Cone::makeSlopeSlice(float currentTheta, float nextTheta) {
    const float step = 1.0f / m_param1;

    for (int i = 0; i < m_param1; ++i) {
        float v0 = i * step;
        float v1 = (i+1) * step;

        float y0 = 0.5f - v0;
        float y1 = 0.5f - v1;
        float r0 = coneRadiusAtY(y0);
        float r1 = coneRadiusAtY(y1);

        glm::vec3 TL(r0 * glm::cos(currentTheta), y0, r0 * glm::sin(currentTheta));
        glm::vec3 TR(r0 * glm::cos(nextTheta), y0, r0 * glm::sin(nextTheta));
        glm::vec3 BL(r1 * glm::cos(currentTheta), y1, r1 * glm::sin(currentTheta));
        glm::vec3 BR(r1 * glm::cos(nextTheta), y1, r1 * glm::sin(nextTheta));

        glm::vec3 nBL = calcNorm(BL);
        glm::vec3 nBR = calcNorm(BR);

        float kEps = 0.0001f;
        bool topRow = glm::length(TL - TR) < kEps;
        glm::vec3 nTL, nTR;

        if (topRow) {
            glm::vec3 Ntip = glm::normalize(nBL + nBR);
            nTL = Ntip;
            nTR = Ntip;
        } else {
            nTL = calcNorm(TL);
            nTR = calcNorm(TR);
        }
        makeSlopeTile(TL, TR, BL, BR, nTL, nTR, nBL, nBR, currentTheta, nextTheta, v0, v1);
    }
}

void Cone::makeWedge(float currentTheta, float nextTheta) {
    makeSlopeSlice(currentTheta, nextTheta);
    makeCapSlice(currentTheta, nextTheta);
}

void Cone::buildVertices() {
    const float thetaStep = glm::radians(360.f / m_param2);

    for (int k = 0; k < m_param2; ++k) {
        float theta0 = k * thetaStep;
        float theta1 = (k+1) * thetaStep;
        makeWedge(theta0, theta1);
    }
}
