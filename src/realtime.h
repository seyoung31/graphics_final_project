#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "utils/sceneparser.h"
#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "shapes/Sphere.h"
#include "shapes/Cube.h"
#include "shapes/Cylinder.h"
#include "shapes/Cone.h"
#include "camera/camera.h"
#include "shader.h"
#include "particles/particles.h"

struct GLShape {
    glm::mat4 model;
    PrimitiveType primType;
    Sphere sphere;
    Cube cube;
    Cylinder cylinder;
    Cone cone;
    SceneMaterial material;
};

struct GLData {
    GLuint vao = 0;
    GLuint vbo = 0;
    std::vector<float> vertex_data;

};

// C++ side light struct (host-side, not sent directly as a block here)
struct GLight {
    int type;             // 0 = directional, 1 = point, 2 = spot
    glm::vec3 color;      // rgb
    glm::vec3 pos;        // world-space position (for point/spot)
    glm::vec3 dir;        // world-space direction (for directional/spot, should be normalized)
    glm::vec3 function;       // attenuation constants

    float angle;    // cosine of inner cone angle (for spot)
    float penumbra;    // cosine of outer cone angle (for spot)
};


class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:


    GLuint m_vbo;    // Stores id of VBO
    GLuint m_vao;    // Stores id of VAO

    glm::mat4 m_model;

    glm::vec4 m_camera_pos;
    Camera m_camera;

    GLight lights[8];
    int numLights;
    std::vector<std::unique_ptr<GLShape>> myPrimitives;
    GLData m_sphere;
    GLData m_cube;
    GLData m_cylinder;
    GLData m_cone;

    glm::mat4 m_proj; //= camera.calcProjectionMatrix(fov, aspect_ratio, near_plane, far_plane);
    glm::mat4 m_view;

    RenderData scene;
    GLuint m_shader;     // Stores id of shader program
    GLuint m_sphere_vbo; // Stores id of vbo
    GLuint m_sphere_vao; // Stores id of vao
    std::vector<float> m_sphereData;

    GLuint m_cube_vbo; // Stores id of vbo
    GLuint m_cube_vao; // Stores id of vao
    std::vector<float> m_cubeData;

    void initGLShape(GLData &shape, std::vector<float> &data);


    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;
};
