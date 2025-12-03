#include "cylinder.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData.clear();
    m_param1 = std::max(1, param1);
    m_param2 = std::max(3, param2);
    setVertexData();
}

static inline void insertVec3(std::vector<float> &data, const glm::vec3 &v) {
    data.push_back(v.x); data.push_back(v.y); data.push_back(v.z);
}

// ===== helpers =====

static inline void topCapTile(std::vector<float> &out,
                              const glm::vec3 &tl, const glm::vec3 &tr,
                              const glm::vec3 &bl, const glm::vec3 &br) {
    const glm::vec3 n(0.f, 1.f, 0.f);
    // tri1: tl -> tr -> br
    insertVec3(out, tl); insertVec3(out, n);
    insertVec3(out, tr); insertVec3(out, n);
    insertVec3(out, br); insertVec3(out, n);
    // tri2: tl -> br -> bl
    insertVec3(out, tl); insertVec3(out, n);
    insertVec3(out, br); insertVec3(out, n);
    insertVec3(out, bl); insertVec3(out, n);
}

static inline void bottomCapTile(std::vector<float>& out,
                                 const glm::vec3& tl, const glm::vec3& tr,
                                 const glm::vec3& bl, const glm::vec3& br) {
    const glm::vec3 n(0.f, -1.f, 0.f);
    // tri1: tl -> br -> tr
    insertVec3(out, tl); insertVec3(out, n);
    insertVec3(out, br); insertVec3(out, n);
    insertVec3(out, tr); insertVec3(out, n);
    // tri2: tl -> bl -> br
    insertVec3(out, tl); insertVec3(out, n);
    insertVec3(out, bl); insertVec3(out, n);
    insertVec3(out, br); insertVec3(out, n);
}

static inline void sideTile(std::vector<float> &out,
                            const glm::vec3 &tl, const glm::vec3 &tr,
                            const glm::vec3 &bl, const glm::vec3 &br) {
    glm::vec3 ntl = glm::normalize(glm::vec3(tl.x, 0.f, tl.z));
    glm::vec3 ntr = glm::normalize(glm::vec3(tr.x, 0.f, tr.z));
    glm::vec3 nbl = glm::normalize(glm::vec3(bl.x, 0.f, bl.z));
    glm::vec3 nbr = glm::normalize(glm::vec3(br.x, 0.f, br.z));

    insertVec3(out, tl); insertVec3(out, ntl);
    insertVec3(out, tr); insertVec3(out, ntr);
    insertVec3(out, br); insertVec3(out, nbr);

    insertVec3(out, tl); insertVec3(out, ntl);
    insertVec3(out, br); insertVec3(out, nbr);
    insertVec3(out, bl); insertVec3(out, nbl);
}

// build one radial slice of the top cap (y = −0.5), outward CCW (normal down)
void makeTopCapSlice(std::vector<float> &out, int radialIdx, int radialCount, int rings) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / radialCount;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    glm::vec3 n(0.f, 1.f, 0.f);
    float y = 0.5f;

    float dr = R / rings;
    for (int i = 0; i < rings; ++i) {
        float r0 = i * dr; // inner radius of ring segment
        float r1 = (i + 1) * dr; // outer radius

        glm::vec3 tl(r0 * std::cos(theta0), y, r0 * std::sin(theta0));
        glm::vec3 tr(r0 * std::cos(theta1), y, r0 * std::sin(theta1));
        glm::vec3 bl(r1 * std::cos(theta0), y, r1 * std::sin(theta0));
        glm::vec3 br(r1 * std::cos(theta1), y, r1 * std::sin(theta1));

        topCapTile(out, tl, tr, bl, br);
    }
}

// build one radial slice of the bottom cap (y = −0.5), outward CCW (normal down)
void makeBottomCapSlice(std::vector<float> &out, int radialIdx, int radialCount, int rings) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / radialCount;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    glm::vec3 n(0.f, -1.f, 0.f);
    float y = -0.5f;

    float dr = R / rings;
    for (int i = 0; i < rings; ++i) {
        float r0 = i * dr;
        float r1 = (i + 1) * dr;

        glm::vec3 tl(r0 * std::cos(theta0), y, r0 * std::sin(theta0));
        glm::vec3 tr(r0 * std::cos(theta1), y, r0 * std::sin(theta1));
        glm::vec3 bl(r1 * std::cos(theta0), y, r1 * std::sin(theta0));
        glm::vec3 br(r1 * std::cos(theta1), y, r1 * std::sin(theta1));

        bottomCapTile(out, tl, tr, bl, br);
    }
}

// one vertical side slice between theta0,theta1 split into stacks
void makeSideSlice(std::vector<float> &out, int radialIdx, int radialCount, int stacks) {
    const float R = 0.5f;
    const float dTheta = glm::two_pi<float>() / radialCount;
    float theta0 = radialIdx * dTheta;
    float theta1 = (radialIdx + 1) * dTheta;

    float dy = 1.f / stacks;
    for (int i = 0; i < stacks; ++i) {
        float v0 = -0.5f + i * dy;
        float v1 = -0.5f + (i + 1) * dy;

        glm::vec3 tl(R * std::cos(theta0), v1, R * std::sin(theta0));
        glm::vec3 tr(R * std::cos(theta1), v1, R * std::sin(theta1));
        glm::vec3 bl(R * std::cos(theta0), v0, R * std::sin(theta0));
        glm::vec3 br(R * std::cos(theta1), v0, R * std::sin(theta1));

        sideTile(out, tl, tr, bl, br);
    }
}

void Cylinder::setVertexData() {
    // reserve (rough) capacity: 6 vertices * (pos+normal=6 floats) * tiles
    // caps: param2 * param1 tiles each; side: param2 * param1 tiles
    size_t tiles = (size_t)m_param2 * (size_t)m_param1 * 3;
    m_vertexData.reserve(m_vertexData.size() + tiles * 6 * 6);

    // build around the circle
    for (int k = 0; k < m_param2; ++k) {
        makeTopCapSlice(m_vertexData, k, m_param2, m_param1);
        makeBottomCapSlice(m_vertexData, k, m_param2, m_param1);
        makeSideSlice(m_vertexData, k, m_param2, m_param1);
    }
}
