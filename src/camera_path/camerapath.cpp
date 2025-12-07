#include "camerapath.h"
#include <algorithm>
#include <iostream>

CameraPath::CameraPath()
    : m_enabled(false)
    , m_looping(true)
    , m_speed(1.0f)
    , m_currentTime(0.0f)
{
}

void CameraPath::addKeyframe(const glm::vec3& position, const glm::vec3& lookAt,
                             const glm::vec3& up, float time) {
    m_keyframes.emplace_back(position, lookAt, up, time);

    // Sort keyframes by time to ensure proper ordering
    std::sort(m_keyframes.begin(), m_keyframes.end(),
              [](const CameraKeyframe& a, const CameraKeyframe& b) {
                  return a.time < b.time;
              });
}

void CameraPath::clear() {
    m_keyframes.clear();
    m_currentTime = 0.0f;
}

void CameraPath::update(float deltaTime) {
    if (!m_enabled || !isValid()) return;

    m_currentTime += deltaTime * m_speed;

    if (m_looping) {
        // Wrap around for looping
        while (m_currentTime > 1.0f) {
            m_currentTime -= 1.0f;
        }
        while (m_currentTime < 0.0f) {
            m_currentTime += 1.0f;
        }
    } else {
        // Clamp to [0, 1] for non-looping
        m_currentTime = glm::clamp(m_currentTime, 0.0f, 1.0f);
    }
}

void CameraPath::evaluate(float t, glm::vec3& outPosition,
                          glm::vec3& outLookAt, glm::vec3& outUp) const {
    if (!isValid()) {
        std::cerr << "CameraPath::evaluate - Invalid path (need at least 2 keyframes)" << std::endl;
        return;
    }

    // Clamp t to valid range
    t = glm::clamp(t, 0.0f, 1.0f);

    // Find which segment we're in
    size_t idx0, idx1;
    float localT;
    findSegment(t, idx0, idx1, localT);

    // Get keyframes for this segment
    const CameraKeyframe& k0 = m_keyframes[idx0];
    const CameraKeyframe& k1 = m_keyframes[idx1];

    // === Position interpolation using cubic Bézier ===
    // Compute control points using tangent-based approach (Buckley 1994)
    glm::vec3 tangent0 = computeTangent(idx0);
    glm::vec3 tangent1 = computeTangent(idx1);

    // Time difference between keyframes
    float dt = k1.time - k0.time;
    if (dt < 0.0001f) dt = 0.0001f; // Prevent division by zero

    // Bézier control points: P0, P1, P2, P3
    glm::vec3 p0 = k0.position;
    glm::vec3 p1 = k0.position + tangent0 * (dt / 3.0f);  // Control point based on tangent
    glm::vec3 p2 = k1.position - tangent1 * (dt / 3.0f);  // Control point based on tangent
    glm::vec3 p3 = k1.position;

    outPosition = cubicBezier(p0, p1, p2, p3, localT);

    // === Orientation interpolation ===
    // For a circular path looking at the center, we can either:
    // 1. Interpolate the lookAt point directly (simple)
    // 2. Use quaternion SLERP for orientation (complex, smooth)

    // Simple approach: linearly interpolate lookAt point
    outLookAt = glm::mix(k0.lookAt, k1.lookAt, localT);

    // For up vector, we can keep it constant or interpolate
    // Since we're orbiting horizontally, keep up as world up
    outUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Alternative: If you want smooth up vector interpolation:
    // outUp = glm::normalize(glm::mix(k0.up, k1.up, localT));
}

void CameraPath::applyToCamera(glm::vec4& cameraPos, glm::vec4& cameraLook,
                               glm::vec4& cameraUp) const {
    if (!m_enabled || !isValid()) return;

    glm::vec3 pos, look, up;
    evaluate(m_currentTime, pos, look, up);

    cameraPos = glm::vec4(pos, 1.0f);
    cameraLook = glm::vec4(look, 1.0f);
    cameraUp = glm::vec4(glm::normalize(up), 0.0f);
}

// === Private Helper Methods ===

void CameraPath::findSegment(float t, size_t& outIdx0, size_t& outIdx1,
                             float& outLocalT) const {
    // Find the two keyframes that bracket time t
    for (size_t i = 0; i < m_keyframes.size() - 1; ++i) {
        if (t >= m_keyframes[i].time && t <= m_keyframes[i + 1].time) {
            outIdx0 = i;
            outIdx1 = i + 1;

            float segmentStart = m_keyframes[i].time;
            float segmentEnd = m_keyframes[i + 1].time;
            float segmentLength = segmentEnd - segmentStart;

            if (segmentLength < 0.0001f) {
                outLocalT = 0.0f;
            } else {
                outLocalT = (t - segmentStart) / segmentLength;
            }
            return;
        }
    }

    // Fallback: use last segment
    outIdx0 = m_keyframes.size() - 2;
    outIdx1 = m_keyframes.size() - 1;
    outLocalT = 1.0f;
}

glm::vec3 CameraPath::cubicBezier(const glm::vec3& p0, const glm::vec3& p1,
                                  const glm::vec3& p2, const glm::vec3& p3,
                                  float t) const {
    // Standard cubic Bézier formula
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
}

glm::vec3 CameraPath::computeTangent(size_t index) const {
    // Catmull-Rom style tangent computation for smooth curves
    size_t n = m_keyframes.size();

    if (n < 2) return glm::vec3(0.0f);

    if (index == 0) {
        // First keyframe: use forward difference
        return (m_keyframes[1].position - m_keyframes[0].position);
    } else if (index == n - 1) {
        // Last keyframe: use backward difference
        return (m_keyframes[n - 1].position - m_keyframes[n - 2].position);
    } else {
        // Middle keyframes: use central difference (Catmull-Rom)
        return 0.5f * (m_keyframes[index + 1].position - m_keyframes[index - 1].position);
    }
}

glm::quat CameraPath::slerpOrientation(const glm::quat& q0, const glm::quat& q1,
                                       float t) const {
    // GLM provides slerp, but we'll implement it explicitly for clarity
    glm::quat qa = glm::normalize(q0);
    glm::quat qb = glm::normalize(q1);

    // Compute dot product
    float cosHalfTheta = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;

    // If qa and qb are very close, do linear interpolation
    if (std::abs(cosHalfTheta) >= 1.0f) {
        return qa;
    }

    // If the dot product is negative, slerp won't take the shorter path
    // Fix by negating one quaternion
    if (cosHalfTheta < 0.0f) {
        qb = -qb;
        cosHalfTheta = -cosHalfTheta;
    }

    // Calculate coefficients
    float halfTheta = std::acos(cosHalfTheta);
    float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

    // If theta is very small, do linear interpolation
    if (std::abs(sinHalfTheta) < 0.001f) {
        return glm::quat(
            qa.w * (1.0f - t) + qb.w * t,
            qa.x * (1.0f - t) + qb.x * t,
            qa.y * (1.0f - t) + qb.y * t,
            qa.z * (1.0f - t) + qb.z * t
            );
    }

    // Standard slerp
    float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
    float ratioB = std::sin(t * halfTheta) / sinHalfTheta;

    return glm::quat(
        qa.w * ratioA + qb.w * ratioB,
        qa.x * ratioA + qb.x * ratioB,
        qa.y * ratioA + qb.y * ratioB,
        qa.z * ratioA + qb.z * ratioB
        );
}

glm::quat CameraPath::vectorsToQuaternion(const glm::vec3& forward,
                                          const glm::vec3& up) const {
    // Build orthonormal basis
    glm::vec3 f = glm::normalize(forward);
    glm::vec3 u = glm::normalize(up);

    // Ensure up is perpendicular to forward
    u = glm::normalize(u - glm::dot(u, f) * f);
    glm::vec3 r = glm::cross(u, f);  // Right vector

    // Construct rotation matrix
    glm::mat3 rotMat;
    rotMat[0] = r;
    rotMat[1] = u;
    rotMat[2] = -f;  // OpenGL uses -Z as forward

    // Convert rotation matrix to quaternion
    return glm::quat_cast(rotMat);
}

void CameraPath::quaternionToVectors(const glm::quat& q, glm::vec3& outForward,
                                     glm::vec3& outUp) const {
    // Convert quaternion to rotation matrix
    glm::mat3 rotMat = glm::mat3_cast(q);

    // Extract basis vectors
    outForward = -rotMat[2];  // -Z axis is forward in OpenGL
    outUp = rotMat[1];        // Y axis is up

    outForward = glm::normalize(outForward);
    outUp = glm::normalize(outUp);
}

glm::vec3 CameraPath::deCasteljau(const std::vector<glm::vec3>& controlPoints,
                                  float t) const {
    // De Casteljau's algorithm for arbitrary degree Bézier curves
    // (Currently using explicit cubic Bézier, but this is here for extensibility)
    if (controlPoints.empty()) return glm::vec3(0.0f);
    if (controlPoints.size() == 1) return controlPoints[0];

    std::vector<glm::vec3> temp = controlPoints;

    while (temp.size() > 1) {
        std::vector<glm::vec3> next;
        for (size_t i = 0; i < temp.size() - 1; ++i) {
            next.push_back(glm::mix(temp[i], temp[i + 1], t));
        }
        temp = next;
    }

    return temp[0];
}
