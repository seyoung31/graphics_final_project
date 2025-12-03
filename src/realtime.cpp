#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "glm/gtc/type_ptr.hpp"
#include "settings.h"
#include "utils/sceneparser.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

#include "src/shaderloader.h"
#include "utils/shaderutils.h"
#include "postprocess.h"

static glm::vec3 rotateAroundAxis(const glm::vec3 &v, const glm::vec3 &axis, float angle) {
    glm::vec3 a = glm::normalize(axis);
    float c = std::cos(angle);
    float s = std::sin(angle);

    return v * c + glm::cross(a, v) * s + a * glm::dot(a, v) * (1.0f - c);
}

// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Clean up post-processing resources
    m_post.destroy();

    this->doneCurrent();
}

void Realtime::initializeGL() {

    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Remember screen/FBO sizes
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;

    // Students: anything requiring OpenGL calls when the program starts should be done here

    // Create shader program
    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert",
                                                 ":/resources/shaders/default.frag");

    // Create VAOs and VBOs for all primitive type
    glGenVertexArrays(PRIM_COUNT, m_vaos);
    glGenBuffers(PRIM_COUNT, m_vbos);

    // Build initial vertex data
    rebuildGeometryFromSettings();

    // Create instance VBOs for per-instance model matrices
    glGenBuffers(PRIM_COUNT, m_instanceVBOs);

    for (int i=0; i < PRIM_COUNT; ++i) {
        glBindVertexArray(m_vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOs[i]);

        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        std::size_t vec4Size = sizeof(glm::vec4);
        GLsizei stride = sizeof(glm::mat4);

        for (int col=0; col < 4; ++col) {
            GLuint attribIndex = 2 + col;
            glEnableVertexAttribArray(attribIndex);
            glVertexAttribPointer(attribIndex, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(col * vec4Size));
            glVertexAttribDivisor(attribIndex, 1);
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_post.init(m_screen_width, m_screen_height,
                QString(":/resources/images/noir_lut_4x4.png"));
    m_glInitialized = true;
}

void Realtime::rebuildGeometryFromSettings() {

    // Get current tessellation params from setting slides
    int p1 = std::max(1, settings.shapeParameter1);
    int p2 = std::max(3, settings.shapeParameter2);

    // Cube
    m_cube.updateParams(p1);
    std::vector<float> cubeData = m_cube.generateShape();
    uploadPrimitive(PRIM_CUBE, cubeData);

    // Sphere
    m_sphere.updateParams(p1, p2);
    std::vector<float> sphereData = m_sphere.generateShape();
    uploadPrimitive(PRIM_SPHERE, sphereData);

    // Cone
    m_cone.updateParams(p1, p2);
    std::vector<float> coneData = m_cone.generateShape();
    uploadPrimitive(PRIM_CONE, coneData);

    // Cylinder
    m_cylinder.updateParams(p1, p2);
    std::vector<float> cylData = m_cylinder.generateShape();
    uploadPrimitive(PRIM_CYLINDER, cylData);
}

void Realtime::uploadPrimitive(PrimitiveIndex idx, const std::vector<float> &data) {

    // How many vertices the primitive has
    m_vertexCounts[idx] = int(data.size() / 6);

    // Bind this primitive's VAO and VBO
    glBindVertexArray(m_vaos[idx]);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[idx]);

    // Copy vertex data into VBO
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), reinterpret_cast<void*>(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Unbind VAO when done
    glBindVertexArray(0);

}

void Realtime::renderScene() {

    // Clear screen color and depth before painting
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate the ishader program
    glUseProgram(m_shader);

    ShaderUtils::uploadCamera(m_shader, m_view, m_proj);
    ShaderUtils::uploadGlobals(m_shader, m_renderData);
    ShaderUtils::uploadLights(m_shader, m_renderData);

    ShaderUtils::drawShapes(m_shader, m_renderData, m_vaos, m_vertexCounts, m_instanceVBOs);

}

void Realtime::paintGL() {
    // Remember which FBO was bound when we entered
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    // 1) Render scene into our offscreen FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_post.fbo());
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScene();

    // 2) Render fullscreen quad into the FBO that was originally bound
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_post.drawToScreen(m_gradeStrength, m_enableColorGrading);

}

void Realtime::resizeGL(int w, int h) {
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;

    // Tells OpenGL how big the screen is
    glViewport(0, 0, m_screen_width, m_screen_height);

    // Match FBO to new window size
    m_post.resize(m_screen_width, m_screen_height);

    updateProjectionMatrix();
}

void Realtime::sceneChanged() {

    makeCurrent();

    RenderData data;
    std::string filePath = settings.sceneFilePath;
    if (SceneParser::parse(filePath, data)) {
        m_renderData = data;
        m_camera = data.cameraData;

        m_nearPlane = settings.nearPlane;
        m_farPlane = settings.farPlane;

        // recompute view & projection matrices
        updateViewMatrix();
        updateProjectionMatrix();

    }

    update(); // asks for a PaintGL() call to occur
}

void Realtime::updateViewMatrix() {

    // Extract eye (camera position), center (look target), and up
    glm::vec3 eye = glm::vec3(m_camera.pos);
    glm::vec3 center = glm::vec3(m_camera.look);
    glm::vec3 up = glm::normalize(glm::vec3(m_camera.up));

    // Build camera basis vectors in world space
    glm::vec3 w = glm::normalize(eye - center);
    glm::vec3 u = glm::normalize(glm::cross(up, w));
    glm::vec3 v = glm::cross(w, u);

    // Change basis from world to camera
    glm::mat4 R(1.f);
    R[0][0] = u.x; R[1][0] = u.y; R[2][0] = u.z; R[3][0] = 0.f;
    R[0][1] = v.x; R[1][1] = v.y; R[2][1] = v.z; R[3][1] = 0.f;
    R[0][2] = w.x; R[1][2] = w.y; R[2][2] = w.z; R[3][2] = 0.f;
    R[0][3] = 0.f; R[1][3] = 0.f; R[2][3] = 0.f; R[3][3] = 1.f;

    // Move the world so that the camera is at the origin
    glm::mat4 T = glm::translate(glm::mat4(1.f), -eye);

    m_view = R * T;
}

void Realtime::updateProjectionMatrix() {

    float theta = m_camera.heightAngle; // in radians
    float f = m_farPlane;
    float n = m_nearPlane;

    // aspect ratio of view port
    int w = width() * m_devicePixelRatio;
    int h = height() * m_devicePixelRatio;
    float aspect = (h == 0) ? 1.f : float(w) / float(h);

    float t = n * std::tan(theta * 0.5f);
    float r = t * aspect;

    // fill perspective matrix
    m_proj = glm::mat4(0.f);

    m_proj[0][0] = n / r;
    m_proj[1][1] = n / t;
    m_proj[2][2] = -(f + n) / (f - n);
    m_proj[2][3] = -1.f;
    m_proj[3][2] = -(2.f * f * n) / (f - n);
    m_proj[3][3] = 0.f;

}

void Realtime::settingsChanged() {

    m_nearPlane = settings.nearPlane;
    m_farPlane = settings.farPlane;

    updateProjectionMatrix();

    if (m_glInitialized) {
        // Rebuild VAOs and VBOs when the shape paramater changes
        makeCurrent();
        rebuildGeometryFromSettings();
        doneCurrent();
    }


    update(); // asks for a PaintGL() call to occur
}

// ================== Camera Movement!

void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        // Use deltaX and deltaY here to rotate
        float sensitivity = 0.002f;

        rotateCamera(-deltaX * sensitivity);
        rotateCameraPitch(-deltaY * sensitivity);

        update(); // asks for a PaintGL() call to occur
    }
}

// Rotate left/right around world up (0, 1, 0)
void Realtime::rotateCamera(float rad) {
    glm::vec3 eye = glm::vec3(m_camera.pos);
    glm::vec3 center = glm::vec3(m_camera.look);
    glm::vec3 up = glm::vec3(m_camera.up);

    glm::vec3 dir = center - eye; // look direction
    glm::vec3 axis(0.f, 1.f, 0.f);

    dir = rotateAroundAxis(dir, axis, rad);
    up = rotateAroundAxis(up, axis, rad);

    m_camera.pos = glm::vec4(eye, 1.f);
    m_camera.look = glm::vec4(eye + dir, 1.f);
    m_camera.up = glm::vec4(glm::normalize(up), 0.f);

    updateViewMatrix();

}

void Realtime::rotateCameraPitch(float rad) {
    glm::vec3 eye = glm::vec3(m_camera.pos);
    glm::vec3 center = glm::vec3(m_camera.look);
    glm::vec3 up = glm::vec3(m_camera.up);

    glm::vec3 dir = center - eye;
    glm::vec3 right = glm::normalize(glm::cross(dir, up));

    dir = rotateAroundAxis(dir,right, rad);
    up  = rotateAroundAxis(up, right, rad);

    m_camera.pos  = glm::vec4(eye, 1.f);
    m_camera.look = glm::vec4(eye + dir, 1.f);
    m_camera.up   = glm::vec4(glm::normalize(up), 0.f);

    updateViewMatrix();
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // Use deltaTime and m_keyMap here to move around

    // Camera basis from current m_camera
    glm::vec3 pos = glm::vec3(m_camera.pos);
    glm::vec3 center = glm::vec3(m_camera.look);
    glm::vec3 up = glm::normalize(glm::vec3(m_camera.up));

    glm::vec3 lookDir = glm::normalize(center - pos);
    glm::vec3 right = glm::normalize(glm::cross(lookDir, up));

    float speed = 5.0f * deltaTime;
    glm::vec3 move(0.f);

    // W/S: forward/back along look direction
    if (m_keyMap[Qt::Key_W]) move += lookDir;
    if (m_keyMap[Qt::Key_S]) move -= lookDir;

    // A/D: left/right along perpendicular axis to look & up
    if (m_keyMap[Qt::Key_A]) move -= right;
    if (m_keyMap[Qt::Key_D]) move += right;

    // Space/Ctrl: world up/down
    if (m_keyMap[Qt::Key_Space])   move += glm::vec3(0.f, 1.f, 0.f);
    if (m_keyMap[Qt::Key_Control]) move -= glm::vec3(0.f, 1.f, 0.f);

    if (glm::length(move) > 0.f) {
        move = glm::normalize(move) * speed;
        pos += move;
        center += move;

        m_camera.pos = glm::vec4(pos, 1.f);
        m_camera.look = glm::vec4(center, 1.f);

        updateViewMatrix();
    }

    update(); // asks for a PaintGL() call to occur
}

void Realtime::tick(QTimerEvent *event) {
    Q_UNUSED(event);
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}
