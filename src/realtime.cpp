#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QImage>
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
    m_particles.cleanup();

    // Clean up texture cache
    cleanupTextures();
    
    // Clean up mesh resources
    cleanupMeshes();

    //clean up shadow fbos
    for (int i = 0; i < 8; i++) {
        if (m_shadow_depth_texs[i]) glDeleteTextures(1, &m_shadow_depth_texs[i]);
        if (m_shadow_fbos[i]) glDeleteFramebuffers(1, &m_shadow_fbos[i]);
        m_shadow_depth_texs[i] = 0;
        m_shadow_fbos[i] = 0;
    }
    
    // Clean up bump mapping resources
    if (m_bumpFBO != 0) {
        glDeleteFramebuffers(1, &m_bumpFBO);
        m_bumpFBO = 0;
    }
    if (m_bumpTexture1 != 0) {
        glDeleteTextures(1, &m_bumpTexture1);
        m_bumpTexture1 = 0;
    }
    if (m_bumpTexture2 != 0) {
        glDeleteTextures(1, &m_bumpTexture2);
        m_bumpTexture2 = 0;
    }
    if (m_bumpDepthRBO != 0) {
        glDeleteRenderbuffers(1, &m_bumpDepthRBO);
        m_bumpDepthRBO = 0;
    }
    if (m_bumpShader != 0) {
        glDeleteProgram(m_bumpShader);
        m_bumpShader = 0;
    }

    this->doneCurrent();
}

void Realtime::cleanupMeshes() {
    for (auto& pair : m_meshVAOs) {
        if (pair.second != 0) {
            glDeleteVertexArrays(1, &pair.second);
        }
    }
    for (auto& pair : m_meshVBOs) {
        if (pair.second != 0) {
            glDeleteBuffers(1, &pair.second);
        }
    }
    for (auto& pair : m_meshInstanceVBOs) {
        if (pair.second != 0) {
            glDeleteBuffers(1, &pair.second);
        }
    }
    m_meshVAOs.clear();
    m_meshVBOs.clear();
    m_meshInstanceVBOs.clear();
    m_meshVertexCounts.clear();
    m_meshLoaders.clear();
    m_meshGroupInfos.clear();
}

void Realtime::loadMesh(const std::string& filepath) {
    // Check if already loaded
    if (m_meshVAOs.find(filepath) != m_meshVAOs.end()) {
        return;
    }
    
    // Load the OBJ file
    ObjLoader loader;
    if (!loader.loadFromFile(filepath)) {
        std::cerr << "Failed to load mesh: " << filepath << std::endl;
        return;
    }
    
    // Get vertex data and upload
    std::vector<float> vertexData = loader.generateShape();
    if (vertexData.empty()) {
        std::cerr << "Mesh has no vertex data: " << filepath << std::endl;
        return;
    }
    
    uploadMesh(filepath, vertexData);
    
    // Store per-group rendering info
    std::vector<MeshGroupInfo> groupInfos;
    int currentVertex = 0;
    for (const ObjMeshGroup& group : loader.getMeshGroups()) {
        MeshGroupInfo info;
        info.startVertex = currentVertex;
        info.vertexCount = static_cast<int>(group.vertices.size());
        info.materialName = group.materialName;
        groupInfos.push_back(info);
        currentVertex += info.vertexCount;
        
        std::cout << "[Mesh] Group '" << group.name << "' material='" << info.materialName 
                  << "' vertices=" << info.vertexCount << std::endl;
    }
    m_meshGroupInfos[filepath] = groupInfos;
    
    // Store the loader for material access
    m_meshLoaders[filepath] = std::move(loader);
}

void Realtime::uploadMesh(const std::string& filepath, const std::vector<float>& data) {
    // 14 floats per vertex: position(3) + normal(3) + uv(2) + tangent(3) + bitangent(3)
    m_meshVertexCounts[filepath] = static_cast<int>(data.size() / 14);
    
    // Create VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    m_meshVAOs[filepath] = vao;
    
    // Create VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);
    m_meshVBOs[filepath] = vbo;
    
    // Bind and upload data
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // UV (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(6*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Tangent (location 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(8*sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // Bitangent (location 4)
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(11*sizeof(GLfloat)));
    glEnableVertexAttribArray(4);
    
    // Create instance VBO for model matrices
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    m_meshInstanceVBOs[filepath] = instanceVBO;
    
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    
    std::size_t vec4Size = sizeof(glm::vec4);
    GLsizei stride = sizeof(glm::mat4);
    
    // Instance model matrix starts at location 5 (after pos, normal, uv, tangent, bitangent)
    for (int col = 0; col < 4; ++col) {
        GLuint attribIndex = 5 + col;
        glEnableVertexAttribArray(attribIndex);
        glVertexAttribPointer(attribIndex, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(col * vec4Size));
        glVertexAttribDivisor(attribIndex, 1);
    }
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    std::cout << "Uploaded mesh: " << filepath << " with " << m_meshVertexCounts[filepath] << " vertices" << std::endl;
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
    m_particles_shader = ShaderLoader::createShaderProgram(":/resources/shaders//particles/particles.vert",
                                                           ":/resources/shaders/particles/particles.frag");

    m_shadow_shader = ShaderLoader::createShaderProgram(":/resources/shaders/shadow.vert",
                                                 ":/resources/shaders/shadow.frag");
    m_bumpShader = ShaderLoader::createShaderProgram(":/resources/shaders/bump.vert",
                                                     ":/resources/shaders/bump.frag");
    std::cout<<"particles shader: " <<m_particles_shader<<std::endl;
    std::cout<<"shadow shader: " <<m_shadow_shader<<std::endl;
    std::cout<<"bump shader: " <<m_bumpShader<<std::endl;

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

        // Instance model matrix starts at location 5 (after pos, normal, uv, tangent, bitangent)
        for (int col=0; col < 4; ++col) {
            GLuint attribIndex = 5 + col;
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

    // Initialize particle system (Particles will create VAO/VBO/texture)
    m_particles.init(m_particles_shader);

    // Load displacement textures
    GLuint waterDisp = loadTexture(":/resources/images/scroll_disp_simple.png");
    // GLuint waterDisp = loadTexture(":/resources/images/water_displacement.png");

    m_water.init(waterDisp);

    //prep shadow fbos for shadows if we use them :)
    makeShadowFBO();
    
    // Initialize bump mapping FBO
    setupBumpMapping();
    
    glClearColor(145.f/255.f, 182.f/255.f, 201.f/255.f, 1.0f);  // dark bluish-gray
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

    // 14 floats per vertex: position(3) + normal(3) + uv(2) + tangent(3) + bitangent(3)
    m_vertexCounts[idx] = int(data.size() / 14);

    // Bind this primitive's VAO and VBO
    glBindVertexArray(m_vaos[idx]);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbos[idx]);

    // Copy vertex data into VBO
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // UV (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(6*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Tangent (location 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(8*sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // Bitangent (location 4)
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14*sizeof(GLfloat), reinterpret_cast<void*>(11*sizeof(GLfloat)));
    glEnableVertexAttribArray(4);

    // Unbind VAO when done
    glBindVertexArray(0);

}

void Realtime::renderScene() {
    // Check if any shape uses bump mapping (primitive shapes or OBJ meshes)
    bool hasBumpMappedShapes = false;
    for (const auto& shape : m_renderData.shapes) {
        // Check primitive shapes
        if (shape.primitive.material.bumpMap.isUsed) {
            hasBumpMappedShapes = true;
            break;
        }
        // Check OBJ meshes - look for normalMap in mesh materials
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH && !shape.primitive.meshfile.empty()) {
            auto it = m_meshLoaders.find(shape.primitive.meshfile);
            if (it != m_meshLoaders.end()) {
                const auto& materials = it->second.getMaterials();
                for (const auto& matPair : materials) {
                    if (!matPair.second.normalMap.empty()) {
                        hasBumpMappedShapes = true;
                        break;
                    }
                }
            }
            if (hasBumpMappedShapes) break;
        }
    }
    
    // Use bump mapping render path if enabled and shapes have bump maps
    if (settings.bumpMapping && hasBumpMappedShapes) {
        redrawbump();
        return;
    }

    // Clear screen color and depth before painting
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shader);

    ShaderUtils::uploadCamera(m_shader, m_view, m_proj);
    ShaderUtils::uploadGlobals(m_shader, m_renderData);
    ShaderUtils::uploadLights(m_shader, m_renderData);

    m_water.apply(m_shader);
    renderShadows();

    ShaderUtils::drawShapes(m_shader, m_renderData, m_vaos, m_vertexCounts, m_instanceVBOs, this);

}

void Realtime::paintGL() {


    // Remember which FBO was bound when we entered
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);


    // 0) SHADOW PASS needs to come first :)
    if (settings.shadowMapping) {
        int i = 0;
        for (const SceneLightData &light : m_renderData.lights) {
            if (i >= 8) break;

            // skip point lights, *but keep index parity*
            if (light.type == LightType::LIGHT_POINT) { i++; continue; }

            // compute and cache VP matrix for this light
            m_lightVPs[i] = getLightVP(light);

            // render depth from light POV into its shadow FBO
            glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_fbos[i]);
            glViewport(0, 0, m_shadow_res, m_shadow_res);
            paintLightView(light, m_lightVPs[i]);

            i++;
        }
    }


    // 1) Render scene into our offscreen FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_post.fbo());
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScene();

    //DO NOT MOVE THIS SNOW CALL -- MUST BE INBETWEEN renderScene() AND 2)
    // let it snow!!!
    if (settings.extraCredit4) {
        // ensure we don't write depth from particles but they still get depth-tested correctly
        glDepthMask(GL_FALSE);
        bool wasCull = glIsEnabled(GL_CULL_FACE);
        if (wasCull) glDisable(GL_CULL_FACE);
        m_particles.render(m_view, m_proj, m_camera.pos);
        if (wasCull) glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
    }

    // 2) Render fullscreen quad into the FBO that was originally bound
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(0, 0, m_screen_width, m_screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_post.drawToScreen(m_gradeStrength,
                        m_enableColorGrading,
                        settings.screenSpaceDOF,
                        settings.nearPlane,
                        settings.farPlane,
                        settings.focusPlane,
                        settings.colorGrading,
                        settings.watercolor,
                        settings.pixelated,
                        settings.pixelSize,
                        settings.isNight);


}

void Realtime::resizeGL(int w, int h) {
    m_screen_width = size().width() * m_devicePixelRatio;
    m_screen_height = size().height() * m_devicePixelRatio;

    // Tells OpenGL how big the screen is
    glViewport(0, 0, m_screen_width, m_screen_height);

    // Match FBO to new window size
    m_post.resize(m_screen_width, m_screen_height);
    
    // Recreate bump mapping FBO with new size
    if (m_bumpFBO != 0) {
        setupBumpMapping();
    }

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
    setupCameraPath();

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

    if (m_glInitialized) {
        makeCurrent();
        if (settings.isNight) {
            glClearColor(7.f/255.f, 18.f/255.f, 46.f/255.f, 1.0f);
        } else {
            glClearColor(145.f/255.f, 182.f/255.f, 201.f/255.f, 1.0f);
        }
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

    m_camera_path.setEnabled(settings.cameraPath);
    m_camera_path.update(deltaTime);
    m_camera_path.applyToCamera(m_camera.pos, m_camera.look, m_camera.up);
    // std::cout<<"camera look: "<< m_camera.look.x<< " " << m_camera.look.y << " "<<  m_camera.look.z <<std::endl;
    // std::cout<<"camera up: "<< m_camera.up.x<< " " << m_camera.up.y << " "<<  m_camera.up.z <<std::endl;

    // Use deltaTime and m_keyMap here to move around

    if (settings.scrollWater){
        m_water.update(deltaTime);
    }


    // Camera basis from current m_camera
    glm::vec3 pos = glm::vec3(m_camera.pos);
    glm::vec3 center = glm::vec3(m_camera.look);
    glm::vec3 up = glm::normalize(glm::vec3(m_camera.up));

    glm::vec3 lookDir = glm::normalize(center - pos);
    glm::vec3 right = -glm::normalize(glm::cross(up, lookDir));

    float speed = 5.0f * deltaTime * settings.moveSpeed;
    glm::vec3 move(0.f);

    // W/S: forward/back along look direction
    if (m_keyMap[Qt::Key_W]) move += lookDir;
    if (m_keyMap[Qt::Key_S]) move -= lookDir;

    // A/D: left/right along perpendicular axis to look & up
    // A moves left (-right), D moves right (+right)
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
    updateViewMatrix();  // This will use the updated camera values


    if (settings.extraCredit4) updateParticleSystem(deltaTime);

    update(); // asks for a PaintGL() call to occur
}

void Realtime::updateParticleSystem(float deltaTime){
    // ---------- REPLACE existing camera-basis & setCameraData block WITH THIS ----------
    // After moving the camera (m_camera.processKeyboard(...)) update view and projection
    updateViewMatrix();
    updateProjectionMatrix();
    // world-space camera position
    glm::vec3 camPos = m_camera.pos;

    // Extract world-space camera basis from view matrix (view maps world -> camera).
    // Right = column0, Up = column1, Forward = -column2
    glm::vec3 camRight   = glm::vec3(m_view[0][0], m_view[1][0], m_view[2][0]);
    glm::vec3 camUp      = glm::vec3(m_view[0][1], m_view[1][1], m_view[2][1]);
    glm::vec3 camForward = -glm::vec3(m_view[0][2], m_view[1][2], m_view[2][2]);

    // Choose a spawn distance that scales with camera frustum so snow appears at a sensible depth.
    // This uses a fraction of the far plane (can be tuned). Clamp so it's never ridiculously near/far.
    float desiredFractionOfFar = 0.25f; // spawn at 25% of the depth range by default
    float spawnDistance = 8.0f; //glm::clamp(settings.nearPlane + (settings.farPlane - settings.nearPlane) * desiredFractionOfFar, 2.0f, 100.0f);
    //NEAR/FAR PLANE
    // If you prefer a fixed spawnDistance, replace the above with: float spawnDistance = 8.0f;

    // Pass camera orientation + projection info to the particle system BEFORE update() so spawn uses the current view
    float fov = m_camera.heightAngle;
    float aspect = ((float) width()) / ((float) height());
    m_particles.setCameraData(camPos, camForward, camRight, camUp, fov, aspect, spawnDistance);

    // Also set camera world position for sorting/distance uses
    m_particles.setCameraPos(camPos);

    m_particles.update(deltaTime);
}

// Example: Set up a simple circular camera path
void Realtime::setupCameraPath() {
    m_camera_path.clear();

    glm::vec3 centerPoint(0.0f, 0.0f, 0.0f);  // Sphere center

    float radius = 35.0f;
    float height = 20.0f;
    int numKeyframes = 64;  // CRITICAL: Need multiple keyframes for circle

    for (int i = 0; i < numKeyframes; ++i) {
        float t = i / float(numKeyframes);  // 0.0, 0.125, 0.25, ... 0.875
        float angle = t * 2.0f * M_PI;      // Full 360° rotation

        glm::vec3 pos(
            centerPoint.x + radius * cos(angle),
            height,
            centerPoint.z + radius * sin(angle)
            );

        glm::vec3 lookAt = centerPoint;  // Always look at sphere
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        m_camera_path.addKeyframe(pos, lookAt, up, t);
    }

    std::cout << "Camera path keyframes:" << std::endl;
    for (size_t i = 0; i < m_camera_path.getKeyframeCount(); ++i) {
        // You'll need to add a getter or make keyframes public temporarily
        std::cout << "Keyframe " << i << ": check your setup" << std::endl;
    }
    m_camera_path.setSpeed(0.1f);
    m_camera_path.setLooping(true);


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


// Texture loading for normal mapping
GLuint Realtime::loadTexture(const std::string& filename) {
    // Check if texture is already loaded
    auto it = m_textureCache.find(filename);
    if (it != m_textureCache.end()) {
        return it->second;
    }
    
    // Load image using Qt
    QImage image(QString::fromStdString(filename));
    if (image.isNull()) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    // Convert to RGBA format
    QImage convertedImage = image.convertToFormat(QImage::Format_RGBA8888);
    
    // Generate OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedImage.width(), convertedImage.height(), 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedImage.bits());
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Cache the texture
    m_textureCache[filename] = textureID;
    
    return textureID;
}

void Realtime::cleanupTextures() {
    for (auto& pair : m_textureCache) {
        if (pair.second != 0) {
            glDeleteTextures(1, &pair.second);
        }
    }
    m_textureCache.clear();
}

//Shadow Logic!!
void Realtime::makeShadowFBO(){
    //purge old stuff
    for (int i = 0; i < 8; i++) {
        if (m_shadow_depth_texs[i] != 0) glDeleteTextures(1, &m_shadow_depth_texs[i]);
        if (m_shadow_fbos[i] != 0) glDeleteFramebuffers(1, &m_shadow_fbos[i]);
        m_shadow_depth_texs[i] = 0;
        m_shadow_fbos[i] = 0;
    }

    m_shadow_res = 16384; //16384 old

    for (int i = 0; i < 8; i++) {

        //DEPTH TEX for shadows
        glGenTextures(1, &m_shadow_depth_texs[i]); //
        glBindTexture(GL_TEXTURE_2D, m_shadow_depth_texs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadow_res, m_shadow_res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Shadow FBO
        glGenFramebuffers(1, &m_shadow_fbos[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_fbos[i]);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, m_shadow_depth_texs[i], 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Shadow FBO " << i << " incomplete, status = " << status << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); //was m_defaultFBO in yali arch, try 0
}

void Realtime::paintLightView(const SceneLightData &light, const glm::mat4 &lightVP) {
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_shadow_shader);

    glUniformMatrix4fv(glGetUniformLocation(m_shadow_shader, "light_view_proj"),
                       1, GL_FALSE, glm::value_ptr(lightVP));

    for (const RenderShapeData &shape : m_renderData.shapes) {
        GLuint vao = 0;
        int vertCount = 0;
        
        switch(shape.primitive.type) {
        case PrimitiveType::PRIMITIVE_SPHERE: 
            vao = m_vaos[PRIM_SPHERE]; 
            vertCount = m_vertexCounts[PRIM_SPHERE]; 
            break;
        case PrimitiveType::PRIMITIVE_CUBE:   
            vao = m_vaos[PRIM_CUBE];   
            vertCount = m_vertexCounts[PRIM_CUBE];   
            break;
        case PrimitiveType::PRIMITIVE_CONE:   
            vao = m_vaos[PRIM_CONE];   
            vertCount = m_vertexCounts[PRIM_CONE];   
            break;
        case PrimitiveType::PRIMITIVE_CYLINDER: 
            vao = m_vaos[PRIM_CYLINDER]; 
            vertCount = m_vertexCounts[PRIM_CYLINDER]; 
            break;
        case PrimitiveType::PRIMITIVE_MESH:
            // Handle mesh primitives
            if (!shape.primitive.meshfile.empty() && hasMesh(shape.primitive.meshfile)) {
                vao = getMeshVAO(shape.primitive.meshfile);
                vertCount = getMeshVertexCount(shape.primitive.meshfile);
            }
            break;
        default: 
            continue;
        }

        if (vao == 0 || vertCount == 0) continue;

        glBindVertexArray(vao);
        glUniformMatrix4fv(glGetUniformLocation(m_shadow_shader, "model_matrix"),
                           1, GL_FALSE, glm::value_ptr(shape.ctm));
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// for light view/proj mat

glm::mat4 Realtime::getLightVP(const SceneLightData& light) {

    if (light.type == LightType::LIGHT_DIRECTIONAL) {
        float backDist = 40.f;   // have to put the light somewhere, as dir doesnt have a position
        float B = 40.f;          // orthogonal vals so we can use half-width/height

        glm::vec3 dir = glm::normalize(glm::vec3(light.dir));

        //look at, f
        glm::vec3 target = glm::vec3(0.f);

        glm::vec3 pos = target - dir * backDist;

        // Up choice with safety fallback:
        glm::vec3 up = glm::vec3(0,1,0);
        if (std::abs(glm::dot(up, dir)) > 0.95f) {
            up = glm::vec3(1,0,0); // avoid near-parallel up
        }

        glm::mat4 V = glm::lookAt(pos, target, up); //view mat

        // float nearL = settings.nearPlane;
        // float farL  = settings.farPlane;

        // glm::mat4 P = glm::ortho(-B, B, -B, B, nearL, farL); //proj mat
        glm::mat4 P = glm::ortho(-B, B, -B, B, 0.f, 80.f); //proj mat

        return P * V;
    }

    if (light.type == LightType::LIGHT_SPOT) {
        glm::vec3 pos = glm::vec3(light.pos);
        glm::vec3 dir = glm::normalize(glm::vec3(light.dir));
        glm::vec3 target = pos + dir;

        glm::vec3 up(0,1,0);
        if (std::abs(glm::dot(up, dir)) > 0.95f) {
            up = glm::vec3(1,0,0);
        }

        glm::mat4 V = glm::lookAt(pos, target, up);

        // IMPORTANT: keep angle in the SAME units the shader is currently using.
        // Shader is using lightAngle[] directly against acos() result.
        // So we mirror that here: treat light.angle as already in "shader units".
        // float fov = light.angle;
        float outerHalf = light.angle;           // radians, half-angle
        float fov = 2.f * outerHalf * 1.05f;     // 5% pad
        fov = glm::clamp(fov, 0.01f, glm::radians(170.f));

        // Safety clamp to avoid invalid perspective FOVs
        fov = glm::clamp(fov, 0.01f, glm::radians(179.0f));

        float aspect = 1.f;
        // float nearL = settings.nearPlane;
        // float farL  = settings.farPlane;
        float nearL = .79;
        float farL = 40.0;

        glm::mat4 P = glm::perspective(fov, aspect, nearL, farL);
        return P * V;
    }

    if (light.type == LightType::LIGHT_POINT) {
        return glm::mat4(1.f); //dont care about this case SOZ!
    }

    return glm::mat4(1.f);
}

void Realtime::renderShadows(){
    GLint useShadowLoc = glGetUniformLocation(m_shader, "use_shadow_mapping");
    glUniform1i(useShadowLoc, settings.shadowMapping);

    if (settings.shadowMapping) {
        int count = std::min((int)m_renderData.lights.size(), 8);

        for (int i = 0; i < count; i++) {
            if (m_renderData.lights[i].type == LightType::LIGHT_POINT) continue;

            // Bind depth texture into a unique unit
            int texUnit = 2 + i;
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, m_shadow_depth_texs[i]);

            // sampler2D shadowMaps[i] = texUnit
            std::string smName = "shadowMaps[" + std::to_string(i) + "]";
            GLint smLoc = glGetUniformLocation(m_shader, smName.c_str());
            glUniform1i(smLoc, texUnit);

            // mat4 lightVP[i] = cached VP
            std::string vpName = "lightVP[" + std::to_string(i) + "]";
            GLint vpLoc = glGetUniformLocation(m_shader, vpName.c_str());
            glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(m_lightVPs[i]));
        }
    }
}

// ================== Bump Mapping (Render-Shift-Subtract) ==================

void Realtime::setupBumpMapping() {
    int width = m_screen_width;
    int height = m_screen_height;
    
    // Clean up existing resources if any
    if (m_bumpFBO != 0) {
        glDeleteFramebuffers(1, &m_bumpFBO);
        m_bumpFBO = 0;
    }
    if (m_bumpTexture1 != 0) {
        glDeleteTextures(1, &m_bumpTexture1);
        m_bumpTexture1 = 0;
    }
    if (m_bumpTexture2 != 0) {
        glDeleteTextures(1, &m_bumpTexture2);
        m_bumpTexture2 = 0;
    }
    if (m_bumpDepthRBO != 0) {
        glDeleteRenderbuffers(1, &m_bumpDepthRBO);
        m_bumpDepthRBO = 0;
    }
    
    // Generate framebuffer
    glGenFramebuffers(1, &m_bumpFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_bumpFBO);
    
    // Create texture for first pass (original UV)
    glGenTextures(1, &m_bumpTexture1);
    glBindTexture(GL_TEXTURE_2D, m_bumpTexture1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Create texture for second pass (shifted UV)
    glGenTextures(1, &m_bumpTexture2);
    glBindTexture(GL_TEXTURE_2D, m_bumpTexture2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &m_bumpDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_bumpDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_bumpDepthRBO);
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

glm::vec2 Realtime::shiftcoords(const glm::vec3& lightDir, const glm::vec3& tangent,
                                 const glm::vec3& bitangent, const glm::vec3& normal, float delta) {
    // Normalize the input vectors
    glm::vec3 T = glm::normalize(tangent);
    glm::vec3 B = glm::normalize(bitangent);
    glm::vec3 N = glm::normalize(normal);
    glm::vec3 L = glm::normalize(lightDir);
    
    // Construct TBN matrix
    glm::mat3 TBN = glm::transpose(glm::mat3(T, B, N));
    
    // Transform light direction to tangent space
    glm::vec3 L_tangent = TBN * L;
    
    glm::vec2 shift;
    shift.x = L_tangent.x * delta;  // Shift in U direction
    shift.y = L_tangent.y * delta;  // Shift in V direction
    
    return shift;
}

void Realtime::redrawbump() {
    // Delta value for texture coordinate shift (adjust bump strength)
    const float delta = 0.005f;
    
    GLint modelLoc = glGetUniformLocation(m_bumpShader, "m_model");
    GLint viewLoc = glGetUniformLocation(m_bumpShader, "view");
    GLint projLoc = glGetUniformLocation(m_bumpShader, "proj");
    GLint uvShiftLoc = glGetUniformLocation(m_bumpShader, "uvShift");
    GLint passModeLoc = glGetUniformLocation(m_bumpShader, "bumpPassMode");
    GLint bumpTexLoc = glGetUniformLocation(m_bumpShader, "BumpTextureSampler");
    GLint modelView3x3Loc = glGetUniformLocation(m_bumpShader, "MV3x3");
    
    // Get material uniform locations (using current project's naming)
    GLint k_a_loc = glGetUniformLocation(m_bumpShader, "k_a");
    GLint k_d_loc = glGetUniformLocation(m_bumpShader, "k_d");
    GLint k_s_loc = glGetUniformLocation(m_bumpShader, "k_s");
    GLint shininess_loc = glGetUniformLocation(m_bumpShader, "shininess");
    
    // Texture uniform locations
    GLint diffuseTextureLoc = glGetUniformLocation(m_bumpShader, "DiffuseTextureSampler");
    GLint useTextureMapLoc = glGetUniformLocation(m_bumpShader, "useTextureMap");
    GLint textureRepeatULoc = glGetUniformLocation(m_bumpShader, "textureRepeatU");
    GLint textureRepeatVLoc = glGetUniformLocation(m_bumpShader, "textureRepeatV");
    
    glUseProgram(m_bumpShader);
    
    // Upload camera, globals, and lights using ShaderUtils
    ShaderUtils::uploadCamera(m_bumpShader, m_view, m_proj);
    ShaderUtils::uploadGlobals(m_bumpShader, m_renderData);
    ShaderUtils::uploadLights(m_bumpShader, m_renderData);
    
    // Upload shadow mapping uniforms
    GLint useShadowLoc = glGetUniformLocation(m_bumpShader, "use_shadow_mapping");
    glUniform1i(useShadowLoc, settings.shadowMapping);
    
    if (settings.shadowMapping) {
        int count = std::min((int)m_renderData.lights.size(), 8);
        for (int i = 0; i < count; i++) {
            if (m_renderData.lights[i].type == LightType::LIGHT_POINT) continue;
            
            int texUnit = 2 + i;
            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(GL_TEXTURE_2D, m_shadow_depth_texs[i]);
            
            std::string smName = "shadowMaps[" + std::to_string(i) + "]";
            GLint smLoc = glGetUniformLocation(m_bumpShader, smName.c_str());
            glUniform1i(smLoc, texUnit);
            
            std::string vpName = "lightVP[" + std::to_string(i) + "]";
            GLint vpLoc = glGetUniformLocation(m_bumpShader, vpName.c_str());
            glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(m_lightVPs[i]));
        }
    }
    
    // Set view matrix for TBN calculations
    GLint viewMatLoc = glGetUniformLocation(m_bumpShader, "view");
    if (viewMatLoc != -1) {
        glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(m_view));
    }
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    for (const auto& shape : m_renderData.shapes) {
        // Only process shapes with bump maps
        if (!shape.primitive.material.bumpMap.isUsed) {
            continue;
        }
        
        const auto& material = shape.primitive.material;
        const auto& global = m_renderData.globalData;
        
        // Set model matrix
        glm::mat4 model = shape.ctm;
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        }
        
        // Set material properties (combine global and material)
        glm::vec3 k_a = global.ka * glm::vec3(material.cAmbient);
        glm::vec3 k_d = global.kd * glm::vec3(material.cDiffuse);
        glm::vec3 k_s = global.ks * glm::vec3(material.cSpecular);
        float shininess = material.shininess;
        if (shininess <= 0.f) shininess = 32.f;
        
        if (k_a_loc != -1) glUniform3fv(k_a_loc, 1, glm::value_ptr(k_a));
        if (k_d_loc != -1) glUniform3fv(k_d_loc, 1, glm::value_ptr(k_d));
        if (k_s_loc != -1) glUniform3fv(k_s_loc, 1, glm::value_ptr(k_s));
        if (shininess_loc != -1) glUniform1f(shininess_loc, shininess);
        
        // Compute ModelView 3x3 matrix
        glm::mat4 modelViewMatrix = m_view * model;
        glm::mat3 modelView3x3Matrix = glm::mat3(modelViewMatrix);
        if (modelView3x3Loc != -1) {
            glUniformMatrix3fv(modelView3x3Loc, 1, GL_FALSE, glm::value_ptr(modelView3x3Matrix));
        }
        
        // Load bump texture (height map)
        GLuint bumpTexture = loadTexture(material.bumpMap.filename);
        if (bumpTexture != 0 && bumpTexLoc != -1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, bumpTexture);
            glUniform1i(bumpTexLoc, 1);
        }
        
        // Load and bind diffuse texture if available
        bool hasValidTexture = false;
        if (material.textureMap.isUsed) {
            GLuint diffuseTexture = loadTexture(material.textureMap.filename);
            if (diffuseTexture != 0 && diffuseTextureLoc != -1) {
                hasValidTexture = true;
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, diffuseTexture);
                glUniform1i(diffuseTextureLoc, 0);
                if (textureRepeatULoc != -1) glUniform1f(textureRepeatULoc, material.textureMap.repeatU);
                if (textureRepeatVLoc != -1) glUniform1f(textureRepeatVLoc, material.textureMap.repeatV);
            }
        }
        if (useTextureMapLoc != -1) glUniform1i(useTextureMapLoc, hasValidTexture ? 1 : 0);
        if (!hasValidTexture) {
            if (textureRepeatULoc != -1) glUniform1f(textureRepeatULoc, 1.0f);
            if (textureRepeatVLoc != -1) glUniform1f(textureRepeatVLoc, 1.0f);
        }
        
        // Get VAO for this shape
        GLuint vao = 0;
        int vertexCount = 0;
        switch (shape.primitive.type) {
            case PrimitiveType::PRIMITIVE_CONE:
                vao = m_vaos[PRIM_CONE];
                vertexCount = m_vertexCounts[PRIM_CONE];
                break;
            case PrimitiveType::PRIMITIVE_CYLINDER:
                vao = m_vaos[PRIM_CYLINDER];
                vertexCount = m_vertexCounts[PRIM_CYLINDER];
                break;
            case PrimitiveType::PRIMITIVE_CUBE:
                vao = m_vaos[PRIM_CUBE];
                vertexCount = m_vertexCounts[PRIM_CUBE];
                break;
            case PrimitiveType::PRIMITIVE_SPHERE:
                vao = m_vaos[PRIM_SPHERE];
                vertexCount = m_vertexCounts[PRIM_SPHERE];
                break;
            case PrimitiveType::PRIMITIVE_MESH:
                // Handle mesh primitives - skip for now, can be added later
                continue;
            default:
                continue;
        }
        
        if (vao == 0 || vertexCount == 0) continue;
        
        glBindVertexArray(vao);
        
        // Single-pass bump mapping - shader computes bump effect from height gradient
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        
        glBindVertexArray(0);
    }
    
    // Now render OBJ meshes with bump mapping
    for (const auto& shape : m_renderData.shapes) {
        if (shape.primitive.type != PrimitiveType::PRIMITIVE_MESH) continue;
        if (shape.primitive.meshfile.empty()) continue;
        
        auto loaderIt = m_meshLoaders.find(shape.primitive.meshfile);
        if (loaderIt == m_meshLoaders.end()) continue;
        
        const ObjLoader& loader = loaderIt->second;
        const auto& materials = loader.getMaterials();
        
        // Check if this mesh has any bump maps
        bool hasBumpMaps = false;
        for (const auto& matPair : materials) {
            if (!matPair.second.normalMap.empty()) {
                hasBumpMaps = true;
                break;
            }
        }
        if (!hasBumpMaps) continue;
        
        GLuint meshVAO = getMeshVAO(shape.primitive.meshfile);
        if (meshVAO == 0) continue;
        
        const auto* groupInfos = getMeshGroupInfos(shape.primitive.meshfile);
        if (groupInfos == nullptr || groupInfos->empty()) continue;
        
        glm::mat4 model = shape.ctm;
        
        // Set model matrix
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        }
        
        // Compute ModelView 3x3 matrix
        glm::mat4 modelViewMatrix = m_view * model;
        glm::mat3 modelView3x3Matrix = glm::mat3(modelViewMatrix);
        if (modelView3x3Loc != -1) {
            glUniformMatrix3fv(modelView3x3Loc, 1, GL_FALSE, glm::value_ptr(modelView3x3Matrix));
        }
        
        glBindVertexArray(meshVAO);
        
        // Render each material group that has a bump map
        for (const auto& groupInfo : *groupInfos) {
            if (groupInfo.vertexCount == 0) continue;
            
            // Find material for this group
            auto matIt = materials.find(groupInfo.materialName);
            if (matIt == materials.end()) continue;
            
            const ObjMaterial& objMat = matIt->second;
            
            // Skip groups without bump maps
            if (objMat.normalMap.empty()) continue;
            
            // Set material properties
            const auto& global = m_renderData.globalData;
            glm::vec3 k_a = global.ka * objMat.ambient;
            glm::vec3 k_d = global.kd * objMat.diffuse;
            glm::vec3 k_s = global.ks * objMat.specular;
            float shiny = objMat.shininess > 0.f ? objMat.shininess : 32.f;
            
            if (k_a_loc != -1) glUniform3fv(k_a_loc, 1, glm::value_ptr(k_a));
            if (k_d_loc != -1) glUniform3fv(k_d_loc, 1, glm::value_ptr(k_d));
            if (k_s_loc != -1) glUniform3fv(k_s_loc, 1, glm::value_ptr(k_s));
            if (shininess_loc != -1) glUniform1f(shininess_loc, shiny);
            
            // Load bump texture (height map) - normalMap contains bump map path
            GLuint bumpTexture = loadTexture(objMat.normalMap);
            if (bumpTexture != 0 && bumpTexLoc != -1) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, bumpTexture);
                glUniform1i(bumpTexLoc, 1);
            }
            
            // Load diffuse texture if available
            bool hasValidTexture = false;
            if (!objMat.diffuseTexture.empty()) {
                GLuint diffuseTexture = loadTexture(objMat.diffuseTexture);
                if (diffuseTexture != 0 && diffuseTextureLoc != -1) {
                    hasValidTexture = true;
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, diffuseTexture);
                    glUniform1i(diffuseTextureLoc, 0);
                    if (textureRepeatULoc != -1) glUniform1f(textureRepeatULoc, 1.0f);
                    if (textureRepeatVLoc != -1) glUniform1f(textureRepeatVLoc, 1.0f);
                }
            }
            if (useTextureMapLoc != -1) glUniform1i(useTextureMapLoc, hasValidTexture ? 1 : 0);
            
            // Single-pass bump mapping - shader computes bump effect from height gradient
            glDrawArrays(GL_TRIANGLES, groupInfo.startVertex, groupInfo.vertexCount);
        }
        
        glBindVertexArray(0);
    }
    
    // Now render shapes without bump maps using the regular shader
    glUseProgram(m_shader);
    ShaderUtils::uploadCamera(m_shader, m_view, m_proj);
    ShaderUtils::uploadGlobals(m_shader, m_renderData);
    ShaderUtils::uploadLights(m_shader, m_renderData);
    
    m_water.apply(m_shader);
    renderShadows();
    
    // Render non-bump-mapped shapes using ShaderUtils
    // For meshes, ShaderUtils will render groups without bump maps
    ShaderUtils::drawShapes(m_shader, m_renderData, m_vaos, m_vertexCounts, m_instanceVBOs, this);
}
