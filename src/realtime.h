#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#include "utils/scenedata.h"
#include "settings.h"
#include "utils/sceneparser.h"
#include "postprocess.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>

#include "shapes/cube.h"
#include "shapes/sphere.h"
#include "shapes/cone.h"
#include "shapes/cylinder.h"

class Realtime : public QOpenGLWidget {
    Q_OBJECT
public:
    // Parsed scene
    RenderData m_renderData;

    // Shape generators
    Cube m_cube;
    Sphere m_sphere;
    Cone m_cone;
    Cylinder m_cylinder;

    bool m_glInitialized = false;

    // VAO/VBO per primitive type
    enum PrimitiveIndex {
        PRIM_CUBE = 0,
        PRIM_SPHERE,
        PRIM_CONE,
        PRIM_CYLINDER,
        PRIM_COUNT
    };

    void rebuildGeometryFromSettings();
    void uploadPrimitive(PrimitiveIndex idx, const std::vector<float> &data);

    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);

    void setSettings(Settings *settings);

    // Recomputes the camera view matrix from m_camera
    void updateViewMatrix();
    void updateProjectionMatrix();

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void renderScene();
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    void rotateCamera(float rad);
    void rotateCameraPitch(float rad);

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;

    // Shader program for rendering
    GLuint m_shader = 0;

    GLuint m_vaos[PRIM_COUNT];
    GLuint m_vbos[PRIM_COUNT];
    int m_vertexCounts[PRIM_COUNT]; // how many vertices in each primitive

    // One instance VBO per primitive type
    GLuint m_instanceVBOs[PRIM_COUNT];

    float m_aspect = 1.f;
    float m_nearPlane, m_farPlane;

    Settings *m_settings = nullptr;
    SceneCameraData m_camera;
    glm::mat4 m_view;
    glm::mat4 m_proj;

    PostProcess m_post;
    int m_screen_width, m_screen_height;

    // Post-processing controls
    float m_gradeStrength = 0.4f; // how strong LUT is
    bool m_enableColorGrading = true;

};
