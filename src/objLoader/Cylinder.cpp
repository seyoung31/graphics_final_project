#include "Cylinder.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>
#include <cmath>

void Cylinder::updateParams(int param1, int param2) {
    m_param1 = std::max(1, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

void Cylinder::makeTopCapSlice(int radialIdx) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / m_param2;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    glm::vec3 n(0.f, 1.f, 0.f);
    float y = 0.5f;

    float dr = R / m_param1;
    for (int i = 0; i < m_param1; ++i) {
        float r0 = i * dr;
        float r1 = (i + 1) * dr;

        glm::vec3 tl(r0 * std::cos(theta0), y, r0 * std::sin(theta0));
        glm::vec3 tr(r0 * std::cos(theta1), y, r0 * std::sin(theta1));
        glm::vec3 bl(r1 * std::cos(theta0), y, r1 * std::sin(theta0));
        glm::vec3 br(r1 * std::cos(theta1), y, r1 * std::sin(theta1));

        // UV coordinates for cap
        float u0 = theta0 / (2.0f * glm::pi<float>());
        float u1 = theta1 / (2.0f * glm::pi<float>());
        float v0_uv = r0 / R;
        float v1_uv = r1 / R;

        // tri1: tl -> tr -> br
        Vertex vert1; vert1.position = tl; vert1.normal = n; vert1.uv = glm::vec2(u0, v0_uv);
        Vertex vert2; vert2.position = tr; vert2.normal = n; vert2.uv = glm::vec2(u1, v0_uv);
        Vertex vert3; vert3.position = br; vert3.normal = n; vert3.uv = glm::vec2(u1, v1_uv);
        m_vertices.push_back(vert1);
        m_vertices.push_back(vert2);
        m_vertices.push_back(vert3);

        // tri2: tl -> br -> bl
        Vertex vert4; vert4.position = tl; vert4.normal = n; vert4.uv = glm::vec2(u0, v0_uv);
        Vertex vert5; vert5.position = br; vert5.normal = n; vert5.uv = glm::vec2(u1, v1_uv);
        Vertex vert6; vert6.position = bl; vert6.normal = n; vert6.uv = glm::vec2(u0, v1_uv);
        m_vertices.push_back(vert4);
        m_vertices.push_back(vert5);
        m_vertices.push_back(vert6);
    }
}

void Cylinder::makeBottomCapSlice(int radialIdx) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / m_param2;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    glm::vec3 n(0.f, -1.f, 0.f);
    float y = -0.5f;

    float dr = R / m_param1;
    for (int i = 0; i < m_param1; ++i) {
        float r0 = i * dr;
        float r1 = (i + 1) * dr;

        glm::vec3 tl(r0 * std::cos(theta0), y, r0 * std::sin(theta0));
        glm::vec3 tr(r0 * std::cos(theta1), y, r0 * std::sin(theta1));
        glm::vec3 bl(r1 * std::cos(theta0), y, r1 * std::sin(theta0));
        glm::vec3 br(r1 * std::cos(theta1), y, r1 * std::sin(theta1));

        // UV coordinates for cap
        float u0 = theta0 / (2.0f * glm::pi<float>());
        float u1 = theta1 / (2.0f * glm::pi<float>());
        float v0_uv = r0 / R;
        float v1_uv = r1 / R;

        // tri1: tl -> br -> tr (flipped winding for bottom)
        Vertex vert1; vert1.position = tl; vert1.normal = n; vert1.uv = glm::vec2(u0, v0_uv);
        Vertex vert2; vert2.position = br; vert2.normal = n; vert2.uv = glm::vec2(u1, v1_uv);
        Vertex vert3; vert3.position = tr; vert3.normal = n; vert3.uv = glm::vec2(u1, v0_uv);
        m_vertices.push_back(vert1);
        m_vertices.push_back(vert2);
        m_vertices.push_back(vert3);

        // tri2: tl -> bl -> br
        Vertex vert4; vert4.position = tl; vert4.normal = n; vert4.uv = glm::vec2(u0, v0_uv);
        Vertex vert5; vert5.position = bl; vert5.normal = n; vert5.uv = glm::vec2(u0, v1_uv);
        Vertex vert6; vert6.position = br; vert6.normal = n; vert6.uv = glm::vec2(u1, v1_uv);
        m_vertices.push_back(vert4);
        m_vertices.push_back(vert5);
        m_vertices.push_back(vert6);
    }
}

void Cylinder::makeSideSlice(int radialIdx) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / m_param2;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    float dy = 1.f / m_param1;
    for (int i = 0; i < m_param1; ++i) {
        float v0 = -0.5f + i * dy;
        float v1 = -0.5f + (i + 1) * dy;

        glm::vec3 tl(R * std::cos(theta0), v1, R * std::sin(theta0));
        glm::vec3 tr(R * std::cos(theta1), v1, R * std::sin(theta1));
        glm::vec3 bl(R * std::cos(theta0), v0, R * std::sin(theta0));
        glm::vec3 br(R * std::cos(theta1), v0, R * std::sin(theta1));

        glm::vec3 ntl = glm::normalize(glm::vec3(tl.x, 0.f, tl.z));
        glm::vec3 ntr = glm::normalize(glm::vec3(tr.x, 0.f, tr.z));
        glm::vec3 nbl = glm::normalize(glm::vec3(bl.x, 0.f, bl.z));
        glm::vec3 nbr = glm::normalize(glm::vec3(br.x, 0.f, br.z));

        // UV coordinates for side
        float u0_uv = theta0 / (2.0f * glm::pi<float>());
        float u1_uv = theta1 / (2.0f * glm::pi<float>());
        float v0_uv = (v0 + 0.5f); // Map y from [-0.5, 0.5] to [0, 1]
        float v1_uv = (v1 + 0.5f);

        // tri1: tl -> tr -> br
        Vertex vert1; vert1.position = tl; vert1.normal = ntl; vert1.uv = glm::vec2(u0_uv, v1_uv);
        Vertex vert2; vert2.position = tr; vert2.normal = ntr; vert2.uv = glm::vec2(u1_uv, v1_uv);
        Vertex vert3; vert3.position = br; vert3.normal = nbr; vert3.uv = glm::vec2(u1_uv, v0_uv);
        m_vertices.push_back(vert1);
        m_vertices.push_back(vert2);
        m_vertices.push_back(vert3);

        // tri2: tl -> br -> bl
        Vertex vert4; vert4.position = tl; vert4.normal = ntl; vert4.uv = glm::vec2(u0_uv, v1_uv);
        Vertex vert5; vert5.position = br; vert5.normal = nbr; vert5.uv = glm::vec2(u1_uv, v0_uv);
        Vertex vert6; vert6.position = bl; vert6.normal = nbl; vert6.uv = glm::vec2(u0_uv, v0_uv);
        m_vertices.push_back(vert4);
        m_vertices.push_back(vert5);
        m_vertices.push_back(vert6);
    }
}

void Cylinder::buildVertices() {
    for (int k = 0; k < m_param2; ++k) {
        makeTopCapSlice(k);
        makeBottomCapSlice(k);
        makeSideSlice(k);
    }
}
