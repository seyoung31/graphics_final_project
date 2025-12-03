#include "cone.h"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

static inline glm::vec3 calcNorm(const glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

static inline glm::vec3 coneTipNormal(const glm::vec3& baseL, const glm::vec3& baseR) {
    glm::vec3 nL = calcNorm(baseL);
    glm::vec3 nR = calcNorm(baseR);
    return glm::normalize(nL + nR);
}

static inline float coneRadiusAtY(float y) {
    return 0.5f * (0.5f - y);
}

void Cone::makeCapTile(glm::vec3 topLeft,
                       glm::vec3 topRight,
                       glm::vec3 bottomLeft,
                       glm::vec3 bottomRight) {
    glm::vec3 n(0.f, -1.f, 0.f);
    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, n);
    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, n);
    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, n);

    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, n);
    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, n);
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, n);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta){

    const float step = 0.5f / m_param1;

    for (int i=0; i < m_param1; ++i) {
        float r0 = i * step;
        float r1 = (i+1) * step;

        glm::vec3 tl(r0 * glm::cos(currentTheta), -0.5f, r0 * glm::sin(currentTheta));
        glm::vec3 tr(r0 * glm::cos(nextTheta), -0.5f, r0 * glm::sin(nextTheta));
        glm::vec3 bl(r1 * glm::cos(currentTheta), -0.5f, r1 * glm::sin(currentTheta));
        glm::vec3 br(r1 * glm::cos(nextTheta), -0.5f, r1 * glm::sin(nextTheta));

        makeCapTile(tl, tr, bl, br);
    }
}

void Cone::makeSlopeTile(glm::vec3 topLeft,
                         glm::vec3 topRight,
                         glm::vec3 bottomLeft,
                         glm::vec3 bottomRight,
                         glm::vec3 nTL, glm::vec3 nTR,
                         glm::vec3 nBL, glm::vec3 nBR) {

    insertVec3(m_vertexData, topLeft); insertVec3(m_vertexData, nTL);
    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, nTR);
    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, nBL);

    insertVec3(m_vertexData, bottomLeft); insertVec3(m_vertexData, nBL);
    insertVec3(m_vertexData, topRight); insertVec3(m_vertexData, nTR);
    insertVec3(m_vertexData, bottomRight); insertVec3(m_vertexData, nBR);
}

void Cone::makeSlopeSlice(float currentTheta, float nextTheta){

    const float step = 1.0f / m_param1;

    for (int i=0; i < m_param1; ++i) {
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
        makeSlopeTile(TL, TR, BL, BR, nTL, nTR, nBL, nBR);
    }

}

void Cone::makeWedge(float currentTheta, float nextTheta) {

    makeSlopeSlice(currentTheta, nextTheta);
    makeCapSlice(currentTheta, nextTheta);

}

void Cone::setVertexData() {

    const float thetaStep = glm::radians(360.f / m_param2);

    for (int k=0; k < m_param2; ++k) {
        float theta0 = k * thetaStep;
        float theta1 = (k+1) * thetaStep;
        makeWedge(theta0, theta1);
    }
}

void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
