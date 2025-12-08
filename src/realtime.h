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
#include <string>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>

#include "shapes/Cube.h"
#include "shapes/Sphere.h"
#include "shapes/Cone.h"
#include "shapes/Cylinder.h"
#include "shapes/ObjLoader.h"

#include "particles/particles.h"
#include "camera_path/camerapath.h"
#include "utils/watereffect.h"

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
        PRIM_COUNT  // Count of built-in primitives (not including meshes)
    };

    // OBJ mesh loading
    void loadMesh(const std::string& filepath);
    void uploadMesh(const std::string& filepath, const std::vector<float>& data);
    
    // Per-group rendering info
    struct MeshGroupInfo {
        int startVertex;
        int vertexCount;
        std::string materialName;
    };
    
    // Mesh VAO/VBO storage (keyed by filepath)
    std::unordered_map<std::string, GLuint> m_meshVAOs;
    std::unordered_map<std::string, GLuint> m_meshVBOs;
    std::unordered_map<std::string, GLuint> m_meshInstanceVBOs;
    std::unordered_map<std::string, int> m_meshVertexCounts;
    std::unordered_map<std::string, ObjLoader> m_meshLoaders;
    std::unordered_map<std::string, std::vector<MeshGroupInfo>> m_meshGroupInfos;

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
    void updateParticleSystem(float deltaTime);
    void setupCameraPath();
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
    GLuint m_bumpShader = 0; // Shader for bump mapping passes

    GLuint m_particles_shader;    //particle shader
    Particles m_particles;        //particle object

    CameraPath m_camera_path;

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

    WaterEffect m_water;

    // Post-processing controls
    float m_gradeStrength = 0.4f; // how strong LUT is
    bool m_enableColorGrading = true;

    //Shadow Mapping
    GLuint m_shadow_shader;
    int m_shadow_res = 2048;
    GLuint m_shadow_fbos[8];
    GLuint m_shadow_depth_texs[8];
    glm::mat4 m_lightVPs[8];

    void makeShadowFBO();
    glm::mat4 getLightVP(const SceneLightData& light);
    void paintLightView(const SceneLightData& light, const glm::mat4& lightVP);
    void renderShadows();

    // Texture loading for normal mapping
    std::unordered_map<std::string, GLuint> m_textureCache;
    void cleanupTextures();

    // Bump mapping (Render-Shift-Subtract) resources
    GLuint m_bumpFBO = 0;           // Framebuffer for bump mapping passes
    GLuint m_bumpTexture1 = 0;      // Texture for first pass (original UV)
    GLuint m_bumpTexture2 = 0;      // Texture for second pass (shifted UV)
    GLuint m_bumpDepthRBO = 0;      // Depth renderbuffer for bump FBO
    
    // Bump mapping functions
    void setupBumpMapping();        // Initialize FBOs and shaders for bump mapping
    glm::vec2 shiftcoords(const glm::vec3& lightDir, const glm::vec3& tangent,
                          const glm::vec3& bitangent, const glm::vec3& normal, float delta);
    void redrawbump();              // Render using accumulation buffering for bump mapping

    float m_time = 0.0f;
    GLuint m_waterColorTex = 0;
    GLuint m_waterDispTex = 0;

public:
    // Public texture loading function (needed by ShaderUtils)
    GLuint loadTexture(const std::string& filename);
    
    // Mesh access for ShaderUtils
    bool hasMesh(const std::string& filepath) const { return m_meshVAOs.find(filepath) != m_meshVAOs.end(); }
    GLuint getMeshVAO(const std::string& filepath) const { 
        auto it = m_meshVAOs.find(filepath); 
        return it != m_meshVAOs.end() ? it->second : 0; 
    }
    GLuint getMeshInstanceVBO(const std::string& filepath) const { 
        auto it = m_meshInstanceVBOs.find(filepath); 
        return it != m_meshInstanceVBOs.end() ? it->second : 0; 
    }
    int getMeshVertexCount(const std::string& filepath) const { 
        auto it = m_meshVertexCounts.find(filepath); 
        return it != m_meshVertexCounts.end() ? it->second : 0; 
    }
    const ObjLoader* getMeshLoader(const std::string& filepath) const {
        auto it = m_meshLoaders.find(filepath);
        return it != m_meshLoaders.end() ? &it->second : nullptr;
    }
    const std::vector<MeshGroupInfo>* getMeshGroupInfos(const std::string& filepath) const {
        auto it = m_meshGroupInfos.find(filepath);
        return it != m_meshGroupInfos.end() ? &it->second : nullptr;
    }
    
private:
    void cleanupMeshes();
};
