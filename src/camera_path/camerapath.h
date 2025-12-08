#ifndef CAMERAPATH_H
#define CAMERAPATH_H
#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>

struct CameraKeyframe {
    glm::vec3 position;    // Camera position
    glm::vec3 lookAt;      // Point the camera is looking at
    glm::vec3 up;          // Up vector
    float time;            // Time at this keyframe (0.0 to 1.0 normalized)

    CameraKeyframe(const glm::vec3& pos, const glm::vec3& look, const glm::vec3& u, float t)
        : position(pos), lookAt(look), up(u), time(t) {}
};

class CameraPath {
public:
    CameraPath();

    // Add a keyframe to the path
    void addKeyframe(const glm::vec3& position, const glm::vec3& lookAt, const glm::vec3& up, float time);

    // Clear all keyframes
    void clear();

    // Evaluate the camera path at normalized time t (0.0 to 1.0)
    // Returns: position, lookAt, up
    void evaluate(float t, glm::vec3& outPosition, glm::vec3& outLookAt, glm::vec3& outUp) const;

    // Check if path is valid (has at least 2 keyframes)
    bool isValid() const { return m_keyframes.size() >= 2; }

    // Get number of keyframes
    size_t getKeyframeCount() const { return m_keyframes.size(); }

    // Enable/disable the path
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Set/get playback speed multiplier
    void setSpeed(float speed) { m_speed = speed; }
    float getSpeed() const { return m_speed; }

    // Set looping behavior
    void setLooping(bool loop) { m_looping = loop; }
    bool isLooping() const { return m_looping; }

    // Update the path timer (call every frame with deltaTime)
    void update(float deltaTime);

    // Get current time position
    float getCurrentTime() const { return m_currentTime; }

    // Reset to beginning
    void reset() { m_currentTime = 0.0f; }

    // Apply current path state to camera vectors
    void applyToCamera(glm::vec4& cameraPos, glm::vec4& cameraLook, glm::vec4& cameraUp) const;

private:
    std::vector<CameraKeyframe> m_keyframes;
    bool m_enabled;
    bool m_looping;
    float m_speed;
    float m_currentTime;  // Current normalized time (0.0 to 1.0)

    // Bézier curve evaluation using De Casteljau's algorithm
    glm::vec3 deCasteljau(const std::vector<glm::vec3>& controlPoints, float t) const;

    // Cubic Bézier interpolation between two keyframes
    glm::vec3 cubicBezier(const glm::vec3& p0, const glm::vec3& p1,
                          const glm::vec3& p2, const glm::vec3& p3, float t) const;

    // Compute tangent vector for smooth interpolation (Catmull-Rom style)
    glm::vec3 computeTangent(size_t index) const;

    // Spherical linear interpolation for orientation (quaternions)
    glm::quat slerpOrientation(const glm::quat& q0, const glm::quat& q1, float t) const;

    // Convert lookAt and up vectors to quaternion
    glm::quat vectorsToQuaternion(const glm::vec3& forward, const glm::vec3& up) const;

    // Convert quaternion back to lookAt and up vectors
    void quaternionToVectors(const glm::quat& q, glm::vec3& outForward, glm::vec3& outUp) const;

    // Find the segment index for a given time t
    void findSegment(float t, size_t& outIdx0, size_t& outIdx1, float& outLocalT) const;
};

#endif // CAMERAPATH_H
