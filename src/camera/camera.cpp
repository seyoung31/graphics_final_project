#include <stdexcept>
#include "camera.h"
#include "utils/scenedata.h"

Camera::Camera(SceneCameraData sceneCameraData){
    myCameraData = sceneCameraData;

    viewMatrix = calcViewMatrix();
    // projMatrix = calcProjectionMatrix();
    invViewMatrix = glm::inverse(viewMatrix);

}

Camera::Camera(){


}

glm::mat4 Camera::calcViewMatrix() const {
    // vector args from labb glm::vec3(1, 4, 5), glm::vec3(-2, -1, -2), glm::vec3(0, 1, 0)


    /*
     * Look vector specifies the direction camera is pointing.
        In some graphics frameworks, look vector is not specified directly but is
        derived using ‘eye’ and ‘center’ camera parameters, which are the camera’s
        position and the point it’s looking at, respectively
     *
     * Orientation is specified by a direction to look in and a non-collinear
        vector Up from which we derive the rotation of the camera about
        Look

        Often points from position to top of camera

        Up-vector is often (0,1,0)
     *
     * pos just position of camera
     *
     */

    glm::vec3 pos = myCameraData.pos;
    glm::vec3 look = myCameraData.look;
    glm::vec3 up = myCameraData.up;

    glm::vec4 column0(1,0,0,0);
    glm::vec4 column1(0,1,0,0);
    glm::vec4 column2(0,0,1,0);
    glm::vec4 column3(-1 * pos[0], -1 * pos[1], -1 * pos[2], 1);

    glm::mat4 M_translate(column0, column1, column2, column3);

    glm::vec3 w;
    glm::vec3 v;
    glm::vec3 u;
    w = -1.0f * glm::normalize(look);

    v = up - glm::dot(up, w)*w;
    v = glm::normalize(v);

    u = glm::cross(v,w);


    glm::vec4 rot0(u[0], v[0], w[0], 0);
    glm::vec4 rot1(u[1],v[1],w[1],0);
    glm::vec4 rot2(u[2],v[2], w[2],0);
    glm::vec4 rot3(0,0,0,1);
    glm::mat4 M_rotate(rot0,rot1,rot2,rot3);
    return M_rotate * M_translate;
}

glm::mat4 Camera::calcProjectionMatrix(float fov, float aspect_ratio, float near_plane, float far_plane) const{
    float c = - near_plane / far_plane;
    float theta_h = fov;
    float theta_w = 2.0f * glm::atan(aspect_ratio * glm::tan(theta_h / 2.0f));
    //adjust these to be glm::vecs into mat
    glm::vec4 col0_zscale(1, 0, 0, 0);
    glm::vec4 col1_zscale(0, 1, 0, 0);
    glm::vec4 col2_zscale(0, 0, -2, 0);
    glm::vec4 col3_zscale(0, 0, -1, 1);
    glm::mat4 z_scale( col0_zscale, col1_zscale, col2_zscale, col3_zscale);

    glm::vec4 col0_unhinge(1, 0, 0, 0);
    glm::vec4 col1_unhinge(0, 1, 0, 0);
    glm::vec4 col2_unhinge(0, 0, 1 / (1 + c), -1);
    glm::vec4 col3_unhinge(0, 0, -c / (1 + c), 0);
    glm::mat4 unhinge( col0_unhinge, col1_unhinge, col2_unhinge, col3_unhinge );

    glm::vec4 col0_scaling( 1 / (far_plane * tan(theta_w / 2)), 0, 0, 0);
    glm::vec4 col1_scaling( 0, 1 / (far_plane * tan(theta_h / 2)), 0, 0);
    glm::vec4 col2_scaling( 0, 0, 1 / far_plane, 0);
    glm::vec4 col3_scaling( 0, 0, 0, 1);


    glm::mat4 scaling(col0_scaling, col1_scaling, col2_scaling, col3_scaling);

    return z_scale * unhinge * scaling;
}

void Camera::updateCameraData(SceneCameraData sceneCameraData) {
    myCameraData = sceneCameraData;
    viewMatrix = calcViewMatrix();
    // projMatrix = calcProjectionMatrix();
    invViewMatrix = glm::inverse(viewMatrix);
}




// Build a 3x3 rotation matrix from axis-angle using Rodrigues' formula (no glm::rotate)
glm::mat3 Camera::rotationMatrixFromAxisAngle(const glm::vec3 &axis_in, float angle) const {
    glm::vec3 axis = glm::normalize(axis_in);
    float c = std::cos(angle);
    float s = std::sin(angle);
    float omc = 1.0f - c;

    // outer product axis * axis^T
    glm::mat3 outer(
        axis.x * axis.x, axis.x * axis.y, axis.x * axis.z,
        axis.y * axis.x, axis.y * axis.y, axis.y * axis.z,
        axis.z * axis.x, axis.z * axis.y, axis.z * axis.z
        );

    // skew-symmetric K
    glm::mat3 K(
        0.0f,   -axis.z,  axis.y,
        axis.z,  0.0f,   -axis.x,
        -axis.y,  axis.x,   0.0f
        );

    glm::mat3 I(1.0f);
    glm::mat3 R = I * c + outer * omc + K * s;
    return R;
}

// Translation: W/S along look, A/D along camera left/right, Space/Ctrl along WORLD_UP.
// Movement speed MUST be 5 world units/sec (use deltaTime).
void Camera::processKeyboard(bool keyW, bool keyS, bool keyA, bool keyD, bool keySpace, bool keyCtrl, float deltaTime) {
    // compute basis
    glm::vec3 look = glm::normalize(myCameraData.look);
    glm::vec3 up   = glm::normalize(myCameraData.up);
    glm::vec3 right = glm::normalize(glm::cross(look, up)); // camera right

    glm::vec3 movement(0.0f);

    float vel = MOVE_SPEED * deltaTime; // 5 units/sec * dt

    if (keyW) movement += look * vel;
    if (keyS) movement -= look * vel;
    if (keyD) movement += right * vel;
    if (keyA) movement -= right * vel;
    if (keySpace) movement += WORLD_UP * vel;
    if (keyCtrl)  movement -= WORLD_UP * vel;

    // apply to position (pos is vec4)
    glm::vec3 pos3 = glm::vec3(myCameraData.pos);
    pos3 += movement;
    myCameraData.pos = glm::vec4(pos3, 1.0f);

    // update derived matrices
    viewMatrix = calcViewMatrix();
    invViewMatrix = glm::inverse(viewMatrix);
}

// Rotation: Mouse X -> yaw about WORLD_UP. Mouse Y -> pitch about camera right.
// Only rotate when leftMouseDown is true.
void Camera::processMouseMove(float deltaX, float deltaY, bool leftMouseDown) {
    if (!leftMouseDown) return;

    // convert pixel deltas to small angles (radians). Keep this simple for now.
    float yaw = - deltaX * MOUSE_TO_RAD;   // negative/positive sign gives direction; tweak later
    float pitch = - deltaY * MOUSE_TO_RAD;

    glm::vec3 look = glm::normalize(myCameraData.look);
    glm::vec3 up   = glm::normalize(myCameraData.up);

    // 1) yaw about world up
    glm::mat3 R_yaw = rotationMatrixFromAxisAngle(WORLD_UP, yaw);
    glm::vec3 look_yawed = glm::normalize(R_yaw * look);
    glm::vec3 up_yawed   = glm::normalize(R_yaw * up);

    // 2) compute camera right after yaw
    glm::vec3 right = glm::normalize(glm::cross(look_yawed, up_yawed));

    // 3) pitch about camera right
    glm::mat3 R_pitch = rotationMatrixFromAxisAngle(right, pitch);
    glm::vec3 look_after = glm::normalize(R_pitch * look_yawed);
    glm::vec3 up_after   = glm::normalize(R_pitch * up_yawed);

    // 4) re-orthonormalize up to be perpendicular to look
    up_after = glm::normalize(up_after - glm::dot(up_after, look_after) * look_after);

    // commit
    myCameraData.look = glm::vec4(look_after, 0.0);
    myCameraData.up = glm::vec4(up_after, 0.0);

    viewMatrix = calcViewMatrix();
    invViewMatrix = glm::inverse(viewMatrix);
}



glm::mat4 Camera::getViewMatrix() const {
    return viewMatrix;
}

glm::mat4 Camera::getInvViewMatrix() const {
    return invViewMatrix;
}

glm::mat4 Camera::getProjMatrix() const {
    return projMatrix;
}

float Camera::getAspectRatio(float width, float height) const {
    // Optional TODO: implement the getter or make your own design
    return width / height;
}

float Camera::getHeightAngle() const {
    // Optional TODO: implement the getter or make your own design
    return myCameraData.heightAngle;
}

glm::vec4 Camera::getCameraPosition() const {
    return myCameraData.pos;
}

float Camera::getFocalLength() const {
    // extra credit Optional TODO: implement the getter or make your own design
    throw std::runtime_error("not implemented");
}

float Camera::getAperture() const {
    // extra credit Optional TODO: implement the getter or make your own design
    throw std::runtime_error("not implemented");
}
