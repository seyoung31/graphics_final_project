#include "Cone.h"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 =  param1;
    m_param2 =  param2;
    setVertexData();
}

// Task 8: create function(s) to make tiles which you can call later on
// Note: Consider your makeTile() functions from Sphere and Cube

// --- centered cap slice for Cone ---
// Creates radial-subdivided base cap between currentTheta and nextTheta.
// The cone base is at y = -height/2 and the apex is at y = +height/2.
// This function uses m_param1 radial rings.
void Cone::makeCapSlice(float currentTheta, float nextTheta) {
    float radius = 0.5f;
    float height = 1.0f;
    float halfH  = 0.5f * height;

    // radial subdivisions controlled by m_param1 (at least 1)
    int radialSteps = std::max(1, m_param1);

    // base cap at y = -halfH with outward normal pointing down (-Y)
    float y = -halfH;
    glm::vec3 desiredNormal = glm::vec3(0.0f, -1.0f, 0.0f);

    for (int rStep = 0; rStep < radialSteps; ++rStep) {
        float t0 = float(rStep) / float(radialSteps);       // inner fraction (0..1)
        float t1 = float(rStep + 1) / float(radialSteps);   // outer fraction

        float r0 = radius * t0;
        float r1 = radius * t1;

        // corners of the radial quad (inner/current, inner/next, outer/current, outer/next)
        glm::vec3 innerA(r0 * cos(currentTheta), y, r0 * sin(currentTheta));
        glm::vec3 innerB(r0 * cos(nextTheta),    y, r0 * sin(nextTheta));
        glm::vec3 outerA(r1 * cos(currentTheta), y, r1 * sin(currentTheta));
        glm::vec3 outerB(r1 * cos(nextTheta),    y, r1 * sin(nextTheta));

        // emit two triangles for this quad, enforcing winding to match desiredNormal
        makeCapTile(outerA, outerB, innerA, innerB, desiredNormal);
    }
}

// --- cap tile with winding enforcement ---
// outerA (topLeft), outerB (topRight), innerA (bottomLeft), innerB (bottomRight)
void Cone::makeCapTile(glm::vec3 outerA,
                       glm::vec3 outerB,
                       glm::vec3 innerA,
                       glm::vec3 innerB,
                       const glm::vec3& desiredNormal) {

    auto emitTriangleWithNormal = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        // compute geometric normal of (a,b,c)
        glm::vec3 triNormal = glm::normalize(glm::cross(b - a, c - a));

        // if triNormal points opposite desiredNormal, swap b and c to flip winding
        if (glm::dot(triNormal, desiredNormal) < 0.0f) {
            glm::vec3 tmp = b;
            b = c;
            c = tmp;
        }

        insertVec3(m_vertexData, a); insertVec3(m_vertexData, desiredNormal);
        insertVec3(m_vertexData, b); insertVec3(m_vertexData, desiredNormal);
        insertVec3(m_vertexData, c); insertVec3(m_vertexData, desiredNormal);
    };

    // Triangle 1: (outerA, outerB, innerA)
    emitTriangleWithNormal(outerA, outerB, innerA);

    // Triangle 2: (innerA, outerB, innerB)
    emitTriangleWithNormal(innerA, outerB, innerB);
}

// --- analytic cone side normal ---
// Return outward normal for a point on the cone side.
// Uses R/height for the Y component so the normal matches cone slope.
glm::vec3 Cone::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}

// --- centered slope slice for Cone ---
// Vertical range runs from y = -halfH (base) to y = +halfH (apex)
void Cone::makeSlopeSlice(float currentTheta, float nextTheta){
    float radius = 0.5f;
    float height = 1.0f;
    float halfH  = 0.5f * height;

    // m_param2 = number of vertical stacks
    for (int i = 0; i < m_param2; i++) {
        float t0 = float(i) / float(m_param2);       // fraction along height (0=base .. 1=apex)
        float t1 = float(i + 1) / float(m_param2);

        // taper radius along height (cone)
        float r0 = radius * (1.0f - t0);
        float r1 = radius * (1.0f - t1);

        // centered vertical positions
        float y0 = -halfH + t0 * height;   // bottom of stack
        float y1 = -halfH + t1 * height;   // top of stack

        glm::vec3 bl(r0 * cos(currentTheta), y0, r0 * sin(currentTheta)); // bottom-left
        glm::vec3 br(r0 * cos(nextTheta),    y0, r0 * sin(nextTheta));    // bottom-right
        glm::vec3 tl(r1 * cos(currentTheta), y1, r1 * sin(currentTheta)); // top-left
        glm::vec3 tr(r1 * cos(nextTheta),    y1, r1 * sin(nextTheta));    // top-right

        // normals: use analytic calcNorm for side points
        glm::vec3 normBL = glm::normalize(calcNorm(bl));
        glm::vec3 normBR = glm::normalize(calcNorm(br));
        glm::vec3 normTL = glm::normalize(calcNorm(tl));
        glm::vec3 normTR = glm::normalize(calcNorm(tr));

        // If we are at the top row (apex), compute apex normals by averaging adjacent base normals
        if (i + 1 == m_param2) {
            // apex position
            glm::vec3 apex(0.0f, halfH, 0.0f);
            glm::vec3 apexNorm = glm::normalize(normBL + normBR); // average of the two rim normals
            // Use apex normal for the TL/TR which correspond to the apex (r1==0 -> tl/tr collapse to apex)
            normTL = apexNorm;
            normTR = apexNorm;
            // override tl/tr positions to be exactly the apex to avoid tiny floating issues
            tl = apex;
            tr = apex;
        }

        // Insert two triangles (TL, TR, BL) and (BL, TR, BR)
        insertVec3(m_vertexData, tl); insertVec3(m_vertexData, normTL);
        insertVec3(m_vertexData, tr); insertVec3(m_vertexData, normTR);
        insertVec3(m_vertexData, bl); insertVec3(m_vertexData, normBL);

        insertVec3(m_vertexData, bl); insertVec3(m_vertexData, normBL);
        insertVec3(m_vertexData, tr); insertVec3(m_vertexData, normTR);
        insertVec3(m_vertexData, br); insertVec3(m_vertexData, normBR);
    }
}


void Cone::makeWedge(float currentTheta, float nextTheta) {
    // Task 10: create a single wedge of the Cone using the
    //          makeCapSlice() and makeSlopeSlice() functions you
    //          implemented in Task 5
    makeCapSlice(currentTheta, nextTheta);   // base
    makeSlopeSlice(currentTheta, nextTheta); // sloped side
}

void Cone::setVertexData() {
    // Task 10: create a full cone using the makeWedge() function you
    //          just implemented
    // Note: think about how param 2 comes into play here!
    float thetaStep = glm::radians(360.0f / m_param1);

    for (int i = 0; i < m_param1; i++) {
        float currentTheta = i * thetaStep;
        float nextTheta    = (i + 1) * thetaStep;
        makeWedge(currentTheta, nextTheta);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
