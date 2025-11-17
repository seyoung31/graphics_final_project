#include "Cylinder.h"

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;;
    setVertexData();
}

// Create a radially-subdivided cap slice between currentTheta and nextTheta.
// This uses m_param1 radial rings (like the cone's behavior you mentioned).
// isTop == true -> top cap at y = height with +Y normal; otherwise bottom cap at y = 0 with -Y normal.
void Cylinder::makeCapSlice(float currentTheta, float nextTheta, bool isTop) {
    float radius = 0.5f;
    float height = 1.0f;
    float halfH  = 0.5f * height;               // center vertically

    int radialSteps = std::max(1, m_param1);
    float y = isTop ? halfH : -halfH;
    glm::vec3 desiredNormal = isTop ? glm::vec3(0.0f, 1.0f, 0.0f)
                                    : glm::vec3(0.0f, -1.0f, 0.0f);

    for (int rStep = 0; rStep < radialSteps; ++rStep) {
        float t0 = float(rStep) / float(radialSteps);
        float t1 = float(rStep + 1) / float(radialSteps);

        float r0 = radius * t0;
        float r1 = radius * t1;

        glm::vec3 innerA(r0 * cos(currentTheta), y, r0 * sin(currentTheta));
        glm::vec3 innerB(r0 * cos(nextTheta),    y, r0 * sin(nextTheta));
        glm::vec3 outerA(r1 * cos(currentTheta), y, r1 * sin(currentTheta));
        glm::vec3 outerB(r1 * cos(nextTheta),    y, r1 * sin(nextTheta));

        makeCapTile(outerA, outerB, innerA, innerB, desiredNormal);
    }
}

// Emits two triangles for a cap tile (quad) and enforces winding so the
// triangle normals match the provided desiredNormal.
// Parameters: outerA (topLeft), outerB (topRight), innerA (bottomLeft), innerB (bottomRight)
void Cylinder::makeCapTile(glm::vec3 outerA,
                           glm::vec3 outerB,
                           glm::vec3 innerA,
                           glm::vec3 innerB,
                           const glm::vec3& desiredNormal) {

    // We will emit triangles (outerA, outerB, innerA) and (innerA, outerB, innerB)
    // but first ensure the winding produces a triangle normal that aligns with desiredNormal.

    // Helper to check and possibly reorder three points so the triangle normal points with desiredNormal
    auto emitTriangleWithNormal = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        // compute geometric normal of (a,b,c)
        glm::vec3 triNormal = glm::normalize(glm::cross(b - a, c - a));

        // if triNormal points opposite desiredNormal, swap b and c to flip winding
        if (glm::dot(triNormal, desiredNormal) < 0.0f) {
            glm::vec3 tmp = b;
            b = c;
            c = tmp;
            // recompute triNormal optional, but not necessary for insertion because we use desiredNormal for normals
        }

        insertVec3(m_vertexData, a); insertVec3(m_vertexData, desiredNormal);
        insertVec3(m_vertexData, b); insertVec3(m_vertexData, desiredNormal);
        insertVec3(m_vertexData, c); insertVec3(m_vertexData, desiredNormal);
    };

    // First triangle covering the outer edge -> centerwards: (outerA, outerB, innerA)
    emitTriangleWithNormal(outerA, outerB, innerA);

    // Second triangle to complete quad: (innerA, outerB, innerB)
    emitTriangleWithNormal(innerA, outerB, innerB);
}

// --- Fixed slope slice for an actual cylinder (constant radius) ---
void Cylinder::makeSlopeSlice(float currentTheta, float nextTheta){
    float radius = 0.5f;
    float height = 1.0f;
    float halfH  = 0.5f * height;

    // m_param2 = number of vertical stacks
    for (int i = 0; i < m_param2; i++) {
        float t0 = float(i) / float(m_param2);
        float t1 = float(i + 1) / float(m_param2);

        float y0 = -halfH + t0 * height;  // start at -halfH
        float y1 = -halfH + t1 * height;  // up to +halfH

        // Cylinder -> radius constant
        float r0 = radius;
        float r1 = radius;

        glm::vec3 bl(r0 * cos(currentTheta), y0, r0 * sin(currentTheta)); // bottom-left
        glm::vec3 br(r0 * cos(nextTheta),    y0, r0 * sin(nextTheta));    // bottom-right
        glm::vec3 tl(r1 * cos(currentTheta), y1, r1 * sin(currentTheta)); // top-left
        glm::vec3 tr(r1 * cos(nextTheta),    y1, r1 * sin(nextTheta));    // top-right

        // radial normals (y = 0)
        glm::vec3 normBL = glm::normalize(glm::vec3(bl.x, 0.0f, bl.z));
        glm::vec3 normBR = glm::normalize(glm::vec3(br.x, 0.0f, br.z));
        glm::vec3 normTL = glm::normalize(glm::vec3(tl.x, 0.0f, tl.z));
        glm::vec3 normTR = glm::normalize(glm::vec3(tr.x, 0.0f, tr.z));

        insertVec3(m_vertexData, tl); insertVec3(m_vertexData, normTL);
        insertVec3(m_vertexData, tr); insertVec3(m_vertexData, normTR);
        insertVec3(m_vertexData, bl); insertVec3(m_vertexData, normBL);

        insertVec3(m_vertexData, bl); insertVec3(m_vertexData, normBL);
        insertVec3(m_vertexData, tr); insertVec3(m_vertexData, normTR);
        insertVec3(m_vertexData, br); insertVec3(m_vertexData, normBR);
    }
}


void Cylinder::makeWedge(float currentTheta, float nextTheta) {
    // Task 10: create a single wedge of the Cone using the
    //          makeCapSlice() and makeSlopeSlice() functions you
    //          in Task 5
    makeCapSlice(currentTheta, nextTheta, false);

    // cylindrical side (your updated function)
    makeSlopeSlice(currentTheta, nextTheta);

    // top cap
    makeCapSlice(currentTheta, nextTheta, true);
}


void Cylinder::setVertexData() {
    // TODO for Project 5: Lights, Camera
    if (m_param1 <= 0) return; // guard against invalid slice count

    float thetaStep = glm::radians(360.0f / float(m_param1));

    for (int i = 0; i < m_param1; i++) {
        float currentTheta = i * thetaStep;
        float nextTheta    = (i + 1) * thetaStep;
        makeWedge(currentTheta, nextTheta);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

glm::vec3 Cylinder::calcNorm(glm::vec3& pt) {
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}
