#pragma once

#include <glm/glm.hpp>
#include "utils/scenedata.h"

// A class representing a virtual camera.

// Feel free to make your own design choices for Camera class, the functions below are all optional / for your convenience.
// You can either implement and use these getters, or make your own design.
// If you decide to make your own design, feel free to delete these as TAs won't rely on them to grade your assignments.

class Camera {
public:
    Camera();
    Camera(SceneCameraData sceneCameraData);
    // Returns the view matrix for the current camera settings.
    // You might also want to define another function that return the inverse of the view matrix.
    glm::mat4 getViewMatrix() const;

    // Returns the aspect ratio of the camera.
    float getAspectRatio(float width, float height) const;

    // Returns the height angle of the camera in RADIANS.
    float getHeightAngle() const;

    void updateCameraData(SceneCameraData sceneCameraData) ;
    void processKeyboard(bool keyW, bool keyS, bool keyA, bool keyD, bool keySpace, bool keyCtrl, float deltaTime);
    void processMouseMove(float deltaX, float deltaY, bool leftMouseDown);


    glm::vec4 getCameraPosition() const;
    glm::mat4 getInvViewMatrix() const;
    glm::mat4 getProjMatrix() const;
    glm::mat4 calcViewMatrix() const;
    glm::mat4 calcProjectionMatrix(float fov, float aspect_ratio, float near_plane, float far_plane) const;

    // Returns the focal length of this camera.
    // This is for the depth of field extra-credit feature only;
    // You can ignore if you are not attempting to implement depth of field.
    float getFocalLength() const;

    // Returns the focal length of this camera.
    // This is for the depth of field extra-credit feature only;
    // You can ignore if you are not attempting to implement depth of field.
    float getAperture() const;

private:
    SceneCameraData myCameraData;
    glm::mat4 viewMatrix;
    glm::mat4 invViewMatrix;
    glm::mat4 projMatrix;
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const float MOVE_SPEED = 5.0f;         // required 5 world units/sec
    const float MOUSE_TO_RAD = 0.0025f;    // small conversion; tweak later if needed

    glm::mat3 rotationMatrixFromAxisAngle(const glm::vec3 &axis, float angle) const;
};
